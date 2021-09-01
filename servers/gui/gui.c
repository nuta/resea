#include "gui.h"
#include <resea/malloc.h>

static struct os_ops *os = NULL;

/// The list of surfaces ordered by the Z index. The frontmost one (i.e. cursor)
/// comes first.
static list_t surfaces;
struct surface *cursor_surface = NULL;
struct surface *wallpaper_surface = NULL;
int screen_width;
int screen_height;

int get_cursor_x(void) {
    return cursor_surface->x;
}

int get_cursor_y(void) {
    return cursor_surface->y;
}

static struct surface *surface_create(int width, int height,
                                      enum surface_type type,
                                      struct surface_ops *ops, void *data) {
    struct surface *surface = malloc(sizeof(*surface));
    surface->canvas = canvas_create(width, height);
    surface->type = type;
    surface->ops = ops;
    surface->data = data;
    surface->x = 0;
    surface->y = 0;
    surface->width = width;
    surface->height = height;
    surface->mouse_down = false;

    list_push_back(&surfaces, &surface->next);
    return surface;
}

/// Returns the surface-specific data. If the `surface` is not a `type`,
/// NULL is returned.
void *surface_get_data(struct surface *surface, enum surface_type type) {
    if (surface->type != type) {
        return NULL;
    }

    return surface->data;
}

void *surface_get_data_or_panic(struct surface *surface,
                                enum surface_type type) {
    void *data = surface_get_data(surface, type);
    ASSERT(data != NULL);
    return data;
}

static void cursor_render(struct surface *surface) {
    struct cursor_data *data = surface_get_data(surface, SURFACE_CURSOR);
    if (!data) {
        return;
    }

    canvas_draw_cursor(surface->canvas, data);
}

static struct surface_ops cursor_ops = {
    .render = cursor_render,
    .global_mouse_move = NULL,
    .global_mouse_up = NULL,
    .mouse_down = NULL,
};

static void wallpaper_render(struct surface *surface) {
    struct wallpaper_data *data = surface_get_data(surface, SURFACE_WALLPAPER);
    if (!data) {
        return;
    }

    canvas_draw_wallpaper(surface->canvas, data);
}

static struct surface_ops wallpaper_ops = {
    .render = wallpaper_render,
    .global_mouse_move = NULL,
    .global_mouse_up = NULL,
    .mouse_down = NULL,
};

static void window_global_mouse_move(struct surface *surface, int screen_x,
                                     int screen_y) {
    struct window_data *data = surface_get_data(surface, SURFACE_WINDOW);
    if (!data) {
        return;
    }

    if (data->being_moved) {
        // The title bar is being dragged. Move the window position.
        int x_diff = screen_x - data->prev_cursor_x;
        int y_diff = screen_y - data->prev_cursor_y;
        surface->x += x_diff;
        surface->y += y_diff;
        data->prev_cursor_x = screen_x;
        data->prev_cursor_y = screen_y;
    }
}

static bool in_window_widget_pos(struct window_data *window,
                                 struct widget *widget, int surface_x,
                                 int surface_y, int *widget_x, int *widget_y) {
    *widget_y = surface_y - WINDOW_TITLE_HEIGHT;
    *widget_x = surface_x;
    int in_widget =
        widget->y <= *widget_y && *widget_y < widget->y + widget->height
        && widget->x <= *widget_x && *widget_x < widget->x + widget->width;
    return in_widget;
}

static bool window_mouse_down(struct surface *surface, int surface_x,
                              int surface_y) {
    struct window_data *window =
        surface_get_data_or_panic(surface, SURFACE_WINDOW);

    if (surface_y < WINDOW_TITLE_HEIGHT && surface_x > 15 /* close button */) {
        // Move this window.
        window->being_moved = true;
        window->prev_cursor_x = get_cursor_x();
        window->prev_cursor_y = get_cursor_y();
    } else if (surface_y > WINDOW_TITLE_HEIGHT) {
        // Click a widget.
        LIST_FOR_EACH (widget, &window->widgets, struct widget, next) {
            int widget_x, widget_y;
            if (widget->ops->mouse_down
                && in_window_widget_pos(window, widget, surface_x, surface_y,
                                        &widget_x, &widget_y)) {
                widget->ops->mouse_down(widget, widget_x - widget->x,
                                        widget_y - widget->y);
                widget->mouse_down = true;
                break;
            }
        }
    }

    return true;
}

static bool window_global_mouse_up(struct surface *surface, int screen_x,
                                   int screen_y) {
    struct window_data *data = surface_get_data(surface, SURFACE_WINDOW);
    if (!data) {
        return false;
    }

    data->being_moved = false;
    return true;
}

static bool window_mouse_up(struct surface *surface, int surface_x,
                            int surface_y) {
    struct window_data *window =
        surface_get_data_or_panic(surface, SURFACE_WINDOW);
    LIST_FOR_EACH (widget, &window->widgets, struct widget, next) {
        int widget_x, widget_y;
        if (in_window_widget_pos(window, widget, surface_x, surface_y,
                                 &widget_x, &widget_y)) {
            if (widget->ops->mouse_up) {
                widget->ops->mouse_up(widget, widget_x - widget->x,
                                      widget_y - widget->y);
            }
        } else if (widget->ops->mouse_outside_up && widget->mouse_down) {
            widget->ops->mouse_outside_up(widget);
        }

        widget->mouse_down = false;
    }

    return true;
}

static void window_mouse_outside_up(struct surface *surface) {
    struct window_data *window =
        surface_get_data_or_panic(surface, SURFACE_WINDOW);
    LIST_FOR_EACH (widget, &window->widgets, struct widget, next) {
        if (widget->ops->mouse_outside_up && widget->mouse_down) {
            widget->ops->mouse_outside_up(widget);
            widget->mouse_down = false;
        }
    }
}

static void window_render(struct surface *surface) {
    struct window_data *data = surface_get_data(surface, SURFACE_WINDOW);
    if (!data) {
        return;
    }

    int widgets_left, widgets_top;
    canvas_draw_window(surface->canvas, data, &widgets_left, &widgets_top);
    LIST_FOR_EACH (widget, &data->widgets, struct widget, next) {
        int x = widgets_left + widget->x;
        int y = widgets_top + widget->y;
        int max_width = surface->width - x;
        int max_height = surface->height - y;
        if (max_width <= 0 || max_height <= 0) {
            WARN("widget position (%d, %d) exceeds the surface size (%d, %d)",
                 x, y, surface->width, surface->height);
            continue;
        }

        widget->ops->render(widget, surface->canvas, x, y, max_width,
                            max_height);
    }
}

void window_set_title(struct surface *surface, const char *title) {
    struct window_data *data = surface_get_data(surface, SURFACE_WINDOW);
    if (!data) {
        return;
    }

    free(data->title);
    data->title = strdup(title);
}

static struct surface_ops window_ops = {
    .render = window_render,
    .global_mouse_move = window_global_mouse_move,
    .global_mouse_up = window_global_mouse_up,
    .mouse_down = window_mouse_down,
    .mouse_up = window_mouse_up,
    .mouse_outside_up = window_mouse_outside_up,
};

static __nullable struct widget *widget_create(struct surface *window,
                                               enum widget_type type,
                                               struct widget_ops *ops,
                                               void *data) {
    struct window_data *window_data = surface_get_data(window, SURFACE_WINDOW);
    if (!window_data) {
        return NULL;
    }

    struct widget *w = malloc(sizeof(*w));
    w->type = type;
    w->ops = ops;
    w->data = data;
    w->x = 0;
    w->y = 0;
    w->height = 0;
    w->width = 0;
    w->mouse_down = false;
    list_push_back(&window_data->widgets, &w->next);
    return w;
}

/// Returns the widget-specific data. If the `widget` is not a `type`,
/// NULL is returned.
void *widget_get_data(struct widget *widget, enum widget_type type) {
    if (widget->type != type) {
        WARN_DBG("%s: type mismatch (expected=%d, actual=%d)", type,
                 widget->type);
        return NULL;
    }

    return widget->data;
}

void *widget_get_data_or_panic(struct widget *widget, enum widget_type type) {
    void *data = widget_get_data(widget, type);
    ASSERT(data != NULL);
    return data;
}

void widget_move(struct widget *widget, int x, int y) {
    widget->x = x;
    widget->y = y;
}

extern struct widget_ops text_widget_ops;

static __nullable struct widget *widget_text_create(struct surface *window) {
    struct text_widget *data = malloc(sizeof(*data));
    data->body = strdup("");
    // data->color = ; TODO:

    struct widget *widget =
        widget_create(window, WIDGET_TEXT, &text_widget_ops, data);
    if (!widget) {
        free(data->body);
        free(data);
        return NULL;
    }

    return widget;
}

error_t widget_text_set_body(struct widget *text_widget, const char *body) {
    struct text_widget *data = widget_get_data(text_widget, WIDGET_TEXT);
    if (!data) {
        return ERR_INVALID_ARG;
    }

    free(data->body);
    data->body = strdup(body);
    return OK;
}

struct widget_ops text_widget_ops = {
    .render = widget_text_render,
};

extern struct widget_ops button_widget_ops;

static __nullable struct widget *widget_button_create(struct surface *window) {
    struct button_widget *data = malloc(sizeof(*data));
    data->state = BUTTON_STATE_NORMAL;
    data->label = strdup("");

    struct widget *widget =
        widget_create(window, WIDGET_BUTTON, &button_widget_ops, data);
    if (!widget) {
        free(data->label);
        free(data);
        return NULL;
    }

    return widget;
}

error_t widget_button_set_label(struct widget *button_widget,
                                const char *body) {
    struct button_widget *data = widget_get_data(button_widget, WIDGET_BUTTON);
    if (!data) {
        return ERR_INVALID_ARG;
    }

    free(data->label);
    data->label = strdup(body);
    return OK;
}

static void widget_button_mouse_down(struct widget *widget, int x, int y) {
    struct button_widget *button =
        widget_get_data_or_panic(widget, WIDGET_BUTTON);
    button->state = BUTTON_STATE_ACTIVE;
}

static void widget_button_mouse_up(struct widget *widget, int x, int y) {
    struct button_widget *button =
        widget_get_data_or_panic(widget, WIDGET_BUTTON);
    button->state = BUTTON_STATE_NORMAL;
}

static void widget_button_mouse_outside_up(struct widget *widget) {
    struct button_widget *button =
        widget_get_data_or_panic(widget, WIDGET_BUTTON);
    button->state = BUTTON_STATE_NORMAL;
}

struct widget_ops button_widget_ops = {
    .render = widget_button_render,
    .mouse_down = widget_button_mouse_down,
    .mouse_up = widget_button_mouse_up,
    .mouse_outside_up = widget_button_mouse_outside_up,
};

void gui_render(void) {
    struct canvas *screen = os->get_back_buffer();
    LIST_FOR_EACH_REV (s, &surfaces, struct surface, next) {
        s->ops->render(s);
        canvas_copy(screen, s->canvas, s->x, s->y);
    }

    os->swap_buffer();
}

void gui_move_mouse(int x_delta, int y_delta, bool clicked_left,
                    bool clicked_right) {
    static bool prev_clicked_left = false;

    // Move the cursor.
    int cursor_x = cursor_surface->x =
        MIN(MAX(0, cursor_surface->x + x_delta), screen_width - 5);
    int cursor_y = cursor_surface->y =
        MIN(MAX(0, cursor_surface->y + y_delta), screen_height - 5);

    // Notify surfaces mouse events from the foremost one until any of them
    // consume the event.
    bool mouse_down = clicked_left && !prev_clicked_left;
    bool mouse_up = !clicked_left && prev_clicked_left;
    bool consumed_mouse_down = false;
    bool consumed_mouse_up = false;
    LIST_FOR_EACH (s, &surfaces, struct surface, next) {
        bool overlaps = s->x <= cursor_x && cursor_x < s->x + s->width
                        && s->y <= cursor_y && cursor_y < s->y + s->height;

        // mouse move
        if (s->ops->global_mouse_move) {
            s->ops->global_mouse_move(s, cursor_x, cursor_y);
        }

        // mouse up (screen-global)
        if (mouse_up && s->ops->global_mouse_up) {
            s->ops->global_mouse_up(s, cursor_x, cursor_y);
        }

        if (overlaps) {
            int local_x = cursor_x - s->x;
            int local_y = cursor_y - s->y;

            // mouse down
            if (mouse_down && !consumed_mouse_down && s->ops->mouse_down) {
                consumed_mouse_down = s->ops->mouse_down(s, local_x, local_y);
                s->mouse_down = true;
            }

            // mouse up
            if (mouse_up && s->mouse_down && !consumed_mouse_up
                && s->ops->mouse_up) {
                consumed_mouse_up = s->ops->mouse_up(s, local_x, cursor_y);
                s->mouse_down = false;
            }
        }

        // mouse up (outside the surface)
        if (mouse_up && s->mouse_down && s->ops->mouse_outside_up) {
            s->ops->mouse_outside_up(s);
            s->mouse_down = false;
        }
    }

    prev_clicked_left = clicked_left;
}

void gui_init(int screen_width_, int screen_height_, struct os_ops *os_) {
    os = os_;
    screen_width = screen_width_;
    screen_height = screen_height_;
    list_init(&surfaces);

    // Mouse cursor.
    struct cursor_data *cursor_data = malloc(sizeof(*cursor_data));
    cursor_data->shape = CURSOR_POINTER;
    cursor_surface = surface_create(ICON_SIZE, ICON_SIZE, SURFACE_CURSOR,
                                    &cursor_ops, cursor_data);

    // Window.
    struct window_data *window_data = malloc(sizeof(*window_data));
    struct surface *window_surface =
        surface_create(screen_width / 2, screen_height / 2, SURFACE_WINDOW,
                       &window_ops, window_data);
    window_data->being_moved = false;
    window_data->title = strdup("Window");
    list_init(&window_data->widgets);
    window_surface->x = 50;
    window_surface->y = 50;

    window_set_title(window_surface, "Hello");

    struct widget *text = widget_text_create(window_surface);
    widget_text_set_body(text, "Hello World!");
    widget_move(text, 5, 5);

    struct widget *button = widget_button_create(window_surface);
    widget_button_set_label(button, "Click Me!");
    widget_move(button, 5, 50);

    // Wallpaper.
    struct wallpaper_data *wallpaper_data = malloc(sizeof(*wallpaper_data));
    wallpaper_surface =
        surface_create(screen_width, screen_height, SURFACE_WALLPAPER,
                       &wallpaper_ops, wallpaper_data);

    canvas_init(os->get_icon_png, os->open_asset_file);
}
