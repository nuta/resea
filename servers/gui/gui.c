#include "gui.h"
#include <resea/malloc.h>

static struct os_ops *os = NULL;

/// The list of toplevel surfaces ordered by the Z index. The frontmost one
/// (i.e. cursor) comes first.
static list_t toplevel_surfaces;
static struct surface *cursor_surface = NULL;
static struct surface *wallpaper_surface = NULL;
static int screen_width;
static int screen_height;
static int cursor_state = CURSOR_STATE_NORMAL;

int get_cursor_x(void) {
    return cursor_surface->x;
}

int get_cursor_y(void) {
    return cursor_surface->y;
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

static struct surface *do_surface_create(enum surface_type type,
                                         struct surface_ops *ops, void *data,
                                         canvas_t canvas) {
    struct surface *surface = malloc(sizeof(*surface));
    surface->canvas = canvas;
    surface->type = type;
    surface->ops = ops;
    surface->data = data;
    surface->x = 0;
    surface->y = 0;
    surface->width = 0;
    surface->height = 0;
    return surface;
}

static struct surface *surface_create(int width, int height,
                                      enum surface_type type,
                                      struct surface_ops *ops, void *data) {
    struct surface *surface =
        do_surface_create(type, ops, data, canvas_create(width, height));
    surface->width = width;
    surface->height = height;
    list_push_back(&toplevel_surfaces, &surface->next);
    return surface;
}

static __nullable struct surface *child_create(struct surface *window_surface,
                                               enum surface_type type,
                                               struct surface_ops *ops,
                                               void *data) {
    struct window_data *window =
        surface_get_data(window_surface, SURFACE_WINDOW);
    if (!window) {
        return NULL;
    }

    struct surface *child =
        do_surface_create(type, ops, data, window_surface->canvas);
    list_push_back(&window->children, &child->next);
    return child;
}

void surface_move(struct surface *surface, int x, int y) {
    surface->x = x;
    surface->y = y;
}

static void cursor_render(struct surface *surface, int x, int y, int max_width,
                          int max_height) {
    struct cursor_data *data = surface_get_data(surface, SURFACE_CURSOR);
    if (!data) {
        return;
    }

    canvas_draw_cursor(surface->canvas, data);
}

static struct surface_ops cursor_ops = {
    .render = cursor_render,
};

static void wallpaper_render(struct surface *surface, int x, int y,
                             int max_width, int max_height) {
    struct wallpaper_data *data = surface_get_data(surface, SURFACE_WALLPAPER);
    if (!data) {
        return;
    }

    canvas_draw_wallpaper(surface->canvas, data);
}

static struct surface_ops wallpaper_ops = {
    .render = wallpaper_render,
};

static bool in_window_child_pos(struct window_data *window,
                                struct surface *child, int surface_x,
                                int surface_y, int *child_x, int *child_y) {
    *child_y = surface_y - WINDOW_TITLE_HEIGHT - child->x;
    *child_x = surface_x - child->x;
    int in_child = child->y <= *child_y && *child_y < child->y + child->height
                   && child->x <= *child_x
                   && *child_x < child->x + child->width;
    return in_child;
}

static bool window_mouse_down(struct surface *surface, int surface_x,
                              int surface_y) {
    struct window_data *window =
        surface_get_data_or_panic(surface, SURFACE_WINDOW);
    LIST_FOR_EACH (child, &window->children, struct surface, next) {
        int child_x, child_y;
        if (in_window_child_pos(window, child, surface_x, surface_y, &child_x,
                                &child_y)) {
            if (child->ops->mouse_down) {
                child->ops->mouse_down(child, child_x, child_y);
            }
        }
    }

    return true;
}

static void window_mouse_up(struct surface *surface, int screen_x,
                            int screen_y) {
    struct window_data *window =
        surface_get_data_or_panic(surface, SURFACE_WINDOW);
    LIST_FOR_EACH (child, &window->children, struct surface, next) {
        if (child->ops->mouse_up) {
            child->ops->mouse_up(child, screen_x, screen_y);
        }
    }
}

static bool window_clicked(struct surface *surface, int surface_x,
                           int surface_y) {
    struct window_data *window =
        surface_get_data_or_panic(surface, SURFACE_WINDOW);
    LIST_FOR_EACH (child, &window->children, struct surface, next) {
        int child_x, child_y;
        if (child->ops->clicked
            && in_window_child_pos(window, child, surface_x, surface_y,
                                   &child_x, &child_y)) {
            child->ops->clicked(child, child_x, child_y);
        }
    }

    return true;
}

static struct surface *window_drag_start(struct surface *surface, int surface_x,
                                         int surface_y) {
    struct window_data *window =
        surface_get_data_or_panic(surface, SURFACE_WINDOW);
    window->dragging_target = WINDOW_DRAG_IGNORED;
    if (surface_y < WINDOW_TITLE_HEIGHT && surface_x > 15 /* close button */) {
        // Drag the title bar: start moving the window.
        window->dragging_target = WINDOW_DRAG_TITLE_BAR;
        window->prev_cursor_x = get_cursor_x();
        window->prev_cursor_y = get_cursor_y();
    } else {
        // Drag a component in the window.
        struct window_data *window =
            surface_get_data_or_panic(surface, SURFACE_WINDOW);
        LIST_FOR_EACH (child, &window->children, struct surface, next) {
            int child_x, child_y;
            if (child->ops->drag_start
                && in_window_child_pos(window, child, surface_x, surface_y,
                                       &child_x, &child_y)) {
                child->ops->drag_start(child, child_x, child_y);
                window->dragging_target = WINDOW_DRAG_CHILD;
                window->dragging_child = child;
            }
        }
    }

    return surface;
}

static void window_drag_move(struct surface *surface, int screen_x,
                             int screen_y) {
    struct window_data *window = surface_get_data(surface, SURFACE_WINDOW);
    if (!window) {
        return;
    }

    switch (window->dragging_target) {
        case WINDOW_DRAG_IGNORED:
            break;
        case WINDOW_DRAG_TITLE_BAR: {
            int x_diff = screen_x - window->prev_cursor_x;
            int y_diff = screen_y - window->prev_cursor_y;
            surface->x += x_diff;
            surface->y += y_diff;
            window->prev_cursor_x = screen_x;
            window->prev_cursor_y = screen_y;
            break;
        }
        case WINDOW_DRAG_CHILD:
            break;
    }
}

static void window_drag_end(struct surface *surface, int screen_x,
                            int screen_y) {
    struct window_data *window =
        surface_get_data_or_panic(surface, SURFACE_WINDOW);
    switch (window->dragging_target) {
        case WINDOW_DRAG_IGNORED:
            break;
        case WINDOW_DRAG_TITLE_BAR:
            break;
        case WINDOW_DRAG_CHILD: {
            struct surface *child = window->dragging_child;
            DEBUG_ASSERT(window->dragging_child);
            if (child->ops->drag_end) {
                child->ops->drag_end(surface, screen_x, screen_y);
            }
            window->dragging_child = NULL;
            break;
        }
    }
}

static void window_render(struct surface *surface, int x, int y, int max_width,
                          int max_height) {
    struct window_data *data = surface_get_data(surface, SURFACE_WINDOW);
    if (!data) {
        return;
    }

    int childs_left, childs_top;
    canvas_draw_window(surface->canvas, data, &childs_left, &childs_top);
    LIST_FOR_EACH (child, &data->children, struct surface, next) {
        int x = childs_left + child->x;
        int y = childs_top + child->y;
        int max_width = surface->width - x;
        int max_height = surface->height - y;
        if (max_width <= 0 || max_height <= 0) {
            WARN("child position (%d, %d) exceeds the surface size (%d, %d)", x,
                 y, surface->width, surface->height);
            continue;
        }

        child->ops->render(child, x, y, max_width, max_height);
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
    .mouse_down = window_mouse_down,
    .mouse_up = window_mouse_up,
    .clicked = window_clicked,
    .drag_start = window_drag_start,
    .drag_move = window_drag_move,
    .drag_end = window_drag_end,
};

__nonnull struct surface *window_create(const char *title, int width,
                                        int height) {
    struct window_data *window_data = malloc(sizeof(*window_data));
    struct surface *window_surface =
        surface_create(width, height, SURFACE_WINDOW, &window_ops, window_data);
    window_data->title = strdup("title");
    window_data->dragging_target = WINDOW_DRAG_IGNORED;
    window_data->dragging_child = NULL;
    window_surface->x = 0;
    window_surface->y = 0;
    list_init(&window_data->children);
    return window_surface;
}

extern struct surface_ops text_surface_ops;

static __nullable struct surface *text_create(struct surface *window) {
    struct text_data *data = malloc(sizeof(*data));
    data->body = strdup("");
    // data->color = ; TODO:

    struct surface *child =
        child_create(window, SURFACE_TEXT, &text_surface_ops, data);
    if (!child) {
        free(data->body);
        free(data);
        return NULL;
    }

    return child;
}

error_t text_set_body(struct surface *text_data, const char *body) {
    struct text_data *data = surface_get_data(text_data, SURFACE_TEXT);
    if (!data) {
        return ERR_INVALID_ARG;
    }

    free(data->body);
    data->body = strdup(body);
    return OK;
}

struct surface_ops text_surface_ops = {
    .render = text_render,
};

extern struct surface_ops button_surface_ops;

static __nullable struct surface *button_create(struct surface *window) {
    struct button_data *data = malloc(sizeof(*data));
    data->state = BUTTON_STATE_NORMAL;
    data->label = strdup("");

    struct surface *child =
        child_create(window, SURFACE_BUTTON, &button_surface_ops, data);
    if (!child) {
        free(data->label);
        free(data);
        return NULL;
    }

    return child;
}

error_t button_set_label(struct surface *button_data, const char *body) {
    struct button_data *data = surface_get_data(button_data, SURFACE_BUTTON);
    if (!data) {
        return ERR_INVALID_ARG;
    }

    free(data->label);
    data->label = strdup(body);
    return OK;
}

static bool button_mouse_down(struct surface *surface, int x, int y) {
    struct button_data *button =
        surface_get_data_or_panic(surface, SURFACE_BUTTON);
    button->state = BUTTON_STATE_ACTIVE;
    return true;
}

static void button_mouse_up(struct surface *surface, int screen_x,
                            int screen_y) {
    struct button_data *button =
        surface_get_data_or_panic(surface, SURFACE_BUTTON);
    button->state = BUTTON_STATE_NORMAL;
}

static bool button_clicked(struct surface *surface, int x, int y) {
    struct button_data *button =
        surface_get_data_or_panic(surface, SURFACE_BUTTON);
    DBG("clicked!");
    return true;
}

struct surface_ops button_surface_ops = {
    .render = button_render,
    .mouse_down = button_mouse_down,
    .mouse_up = button_mouse_up,
    .clicked = button_clicked,
};

void gui_render(void) {
    struct canvas *screen = os->get_back_buffer();
    LIST_FOR_EACH_REV (s, &toplevel_surfaces, struct surface, next) {
        // FIXME:
        s->ops->render(s, 0, 0, 0, 0);
        canvas_copy(screen, s->canvas, s->x, s->y);
    }

    os->swap_buffer();
}

void gui_move_mouse(int x_delta, int y_delta, bool clicked_left,
                    bool clicked_right) {
    static bool prev_clicked_left = false;
    static struct surface *dragging_surface = NULL;

    // Move the cursor.
    int cursor_x = cursor_surface->x =
        MIN(MAX(0, cursor_surface->x + x_delta), screen_width - 5);
    int cursor_y = cursor_surface->y =
        MIN(MAX(0, cursor_surface->y + y_delta), screen_height - 5);

    bool mouse_down = clicked_left && !prev_clicked_left;
    bool mouse_up = !clicked_left && prev_clicked_left;
    bool drag_start =
        clicked_left && !mouse_up && cursor_state == CURSOR_STATE_CLICKING;
    bool drag_end =
        clicked_left && mouse_up && cursor_state == CURSOR_STATE_DRAGGING;
    bool clicked = mouse_up && cursor_state == CURSOR_STATE_CLICKING;

    if (drag_start) {
        cursor_state = CURSOR_STATE_DRAGGING;
    }

    if (cursor_state == CURSOR_STATE_NORMAL && mouse_down) {
        cursor_state = CURSOR_STATE_CLICKING;
    }

    if (cursor_state != CURSOR_STATE_NORMAL && mouse_up) {
        cursor_state = CURSOR_STATE_NORMAL;
    }

    // drag move
    if (cursor_state == CURSOR_STATE_DRAGGING && !drag_start && !drag_end) {
        DEBUG_ASSERT(dragging_surface != NULL);
        dragging_surface->ops->drag_move(dragging_surface, cursor_x, cursor_y);
    }

    // drag end
    if (drag_end) {
        DEBUG_ASSERT(dragging_surface != NULL);
        dragging_surface->ops->drag_end(dragging_surface, cursor_x, cursor_y);
    }

    // Notify toplevel_surfaces mouse events from the foremost one until any
    // of them consume the event.
    bool consumed_mouse_down = false;
    bool consumed_clicked = false;
    bool consumed_drag_start = false;
    LIST_FOR_EACH (s, &toplevel_surfaces, struct surface, next) {
        bool overlaps = s->x <= cursor_x && cursor_x < s->x + s->width
                        && s->y <= cursor_y && cursor_y < s->y + s->height;

        if (overlaps) {
            int local_x = cursor_x - s->x;
            int local_y = cursor_y - s->y;

            // drag start
            if (drag_start && !consumed_drag_start && s->ops->drag_start) {
                dragging_surface = s->ops->drag_start(s, local_x, local_y);
                consumed_drag_start = true;
            }

            // mouse down
            if (mouse_down && !consumed_mouse_down && s->ops->mouse_down) {
                consumed_mouse_down = s->ops->mouse_down(s, local_x, local_y);
            }

            // clicked
            if (clicked && !consumed_clicked && s->ops->clicked) {
                consumed_clicked = s->ops->clicked(s, local_x, local_y);
            }
        }

        // mouse up
        if (mouse_up && s->ops->mouse_up) {
            s->ops->mouse_up(s, cursor_x, cursor_y);
        }
    }

    prev_clicked_left = clicked_left;
}

void gui_init(int screen_width_, int screen_height_, struct os_ops *os_) {
    os = os_;
    screen_width = screen_width_;
    screen_height = screen_height_;
    list_init(&toplevel_surfaces);

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
    window_data->title = strdup("Window");
    window_data->dragging_target = WINDOW_DRAG_IGNORED;
    window_data->dragging_child = NULL;
    list_init(&window_data->children);
    window_surface->x = 50;
    window_surface->y = 50;

    window_set_title(window_surface, "Hello");

    struct surface *text = text_create(window_surface);
    text_set_body(text, "Hello World!");
    surface_move(text, 5, 5);

    struct surface *button = button_create(window_surface);
    button_set_label(button, "Click Me!");
    surface_move(button, 5, 50);

    // Wallpaper.
    struct wallpaper_data *wallpaper_data = malloc(sizeof(*wallpaper_data));
    wallpaper_surface =
        surface_create(screen_width, screen_height, SURFACE_WALLPAPER,
                       &wallpaper_ops, wallpaper_data);

    canvas_init(os->get_icon_png, os->open_asset_file);
}
