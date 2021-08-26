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
                                      struct surface_ops *ops,
                                      void *user_data) {
    struct surface *surface = malloc(sizeof(*surface));
    surface->canvas = canvas_create(width, height);
    surface->ops = ops;
    surface->user_data = user_data;
    surface->x = 0;
    surface->y = 0;
    surface->width = width;
    surface->height = height;

    list_push_back(&surfaces, &surface->next);
    return surface;
}

static void cursor_render(struct surface *surface) {
    struct cursor_data *data = surface->user_data;
    canvas_draw_cursor(surface->canvas, data);
}

static struct surface_ops cursor_ops = {
    .render = cursor_render,
    .global_mouse_move = NULL,
    .global_mouse_up = NULL,
    .mouse_down = NULL,
};

static void wallpaper_render(struct surface *surface) {
    struct wallpaper_data *data = surface->user_data;
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
    struct window_data *data = surface->user_data;

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

static bool window_mouse_down(struct surface *surface, int x, int y) {
    struct window_data *data = surface->user_data;

    if (y < WINDOW_TITLE_HEIGHT && x > 15 /* close button */) {
        // Move this window.
        data->being_moved = true;
        data->prev_cursor_x = get_cursor_x();
        data->prev_cursor_y = get_cursor_y();
    } else if (y > WINDOW_TITLE_HEIGHT) {
        // Click a widget.
        LIST_FOR_EACH (widget, &data->widgets, struct widget, next) {
            int widget_y = y - WINDOW_TITLE_HEIGHT;
            int widget_x = x;
            if (widget->y <= widget_y && widget_y < widget->y + widget->height
                && widget->x <= widget_x
                && widget_x < widget->x + widget->width) {
                widget->ops->mouse_down(widget, widget_x - widget->x,
                                        widget_y - widget->y);
                break;
            }
        }
    }

    return true;
}

static bool window_global_mouse_up(struct surface *surface, int screen_x,
                                   int screen_y) {
    struct window_data *data = surface->user_data;
    data->being_moved = false;
    return true;
}

static void window_render(struct surface *surface) {
    struct window_data *data = surface->user_data;
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

static struct surface_ops window_ops = {
    .render = window_render,
    .global_mouse_move = window_global_mouse_move,
    .global_mouse_up = window_global_mouse_up,
    .mouse_down = window_mouse_down,
};

struct widget_ops text_widget_ops = {
    .render = widget_text_render,
    .mouse_down = NULL,
    .mouse_up = NULL,
};

static struct widget *widget_create(struct widget_ops *ops, void *data) {
    struct widget *w = malloc(sizeof(*w));
    w->ops = ops;
    w->data = data;
    w->x = 0;
    w->y = 0;
    w->height = 0;
    w->width = 0;
    list_nullify(&w->next);
    return w;
}

static struct widget *widget_text_create(void) {
    struct text_widget *data = malloc(sizeof(*data));
    data->body = strdup("");
    // data->color = ; TODO:
    return widget_create(&text_widget_ops, data);
}

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

        if (s->ops->global_mouse_move) {
            s->ops->global_mouse_move(s, cursor_x, cursor_y);
        }

        if (mouse_up && !consumed_mouse_up && s->ops->global_mouse_up) {
            consumed_mouse_up = s->ops->global_mouse_up(s, cursor_x, cursor_y);
        }

        if (overlaps) {
            int local_x = cursor_x - s->x;
            int local_y = cursor_y - s->y;

            if (mouse_down && !consumed_mouse_down && s->ops->mouse_down) {
                consumed_mouse_down = s->ops->mouse_down(s, local_x, local_y);
            }
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
    cursor_surface =
        surface_create(ICON_SIZE, ICON_SIZE, &cursor_ops, cursor_data);

    // Window.
    struct window_data *window_data = malloc(sizeof(*window_data));
    struct surface *window_surface = surface_create(
        screen_width / 2, screen_height / 2, &window_ops, window_data);
    window_data->being_moved = false;
    window_surface->x = 50;
    window_surface->y = 50;

    // Wallpaper.
    struct wallpaper_data *wallpaper_data = malloc(sizeof(*wallpaper_data));
    wallpaper_surface = surface_create(screen_width, screen_height,
                                       &wallpaper_ops, wallpaper_data);

    canvas_init(os->get_icon_png, os->open_asset_file);
}
