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
    return cursor_surface->screen_x;
}

int get_cursor_y(void) {
    return cursor_surface->screen_y;
}

static struct surface *surface_create(int width, int height,
                                      struct surface_ops *ops,
                                      void *user_data) {
    struct surface *surface = malloc(sizeof(*surface));
    surface->canvas = canvas_create(width, height);
    surface->ops = ops;
    surface->user_data = user_data;
    surface->screen_x = 0;
    surface->screen_y = 0;
    surface->width = width;
    surface->height = height;

    list_push_back(&surfaces, &surface->next);
    return surface;
}

static void cursor_render(struct surface *surface) {
    struct cursor_data *data = surface->user_data;
    canvas_draw_cursor(surface->canvas, data->shape);
}

static struct surface_ops cursor_ops = {
    .render = cursor_render,
    .global_mouse_move = NULL,
    .global_mouse_up = NULL,
    .mouse_down = NULL,
};

static void wallpaper_render(struct surface *surface) {
    struct wallpaper_data *data = surface->user_data;
    canvas_draw_wallpaper(surface->canvas);
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
        surface->screen_x += x_diff;
        surface->screen_y += y_diff;
        data->prev_cursor_x = screen_x;
        data->prev_cursor_y = screen_y;
    }
}

static bool window_mouse_down(struct surface *surface, int x, int y) {
    struct window_data *data = surface->user_data;

    if (y < WINDOW_TITLE_HEIGHT && x > 15 /* close button */) {
        data->being_moved = true;
        data->prev_cursor_x = get_cursor_x();
        data->prev_cursor_y = get_cursor_y();
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
    canvas_draw_window(surface->canvas, data);
}

static struct surface_ops window_ops = {
    .render = window_render,
    .global_mouse_move = window_global_mouse_move,
    .global_mouse_up = window_global_mouse_up,
    .mouse_down = window_mouse_down,
};

void gui_render(void) {
    struct canvas *screen = os->get_back_buffer();
    LIST_FOR_EACH_REV (s, &surfaces, struct surface, next) {
        s->ops->render(s);
        canvas_copy(screen, s->canvas, s->screen_x, s->screen_y);
    }

    os->swap_buffer();
}

void gui_move_mouse(int x_delta, int y_delta, bool clicked_left,
                    bool clicked_right) {
    static bool prev_clicked_left = false;

    // Move the cursor.
    int cursor_x = cursor_surface->screen_x =
        MIN(MAX(0, cursor_surface->screen_x + x_delta), screen_width - 5);
    int cursor_y = cursor_surface->screen_y =
        MIN(MAX(0, cursor_surface->screen_y + y_delta), screen_height - 5);

    // Notify surfaces mouse events from the foremost one until any of them
    // consume the event.
    bool mouse_down = clicked_left && !prev_clicked_left;
    bool mouse_up = !clicked_left && prev_clicked_left;
    bool consumed_mouse_down = false;
    bool consumed_mouse_up = false;
    LIST_FOR_EACH (s, &surfaces, struct surface, next) {
        bool overlaps =
            s->screen_x <= cursor_x && cursor_x < s->screen_x + s->width
            && s->screen_y <= cursor_y && cursor_y < s->screen_y + s->height;

        if (s->ops->global_mouse_move) {
            s->ops->global_mouse_move(s, cursor_x, cursor_y);
        }

        if (mouse_up && !consumed_mouse_up && s->ops->global_mouse_up) {
            consumed_mouse_up = s->ops->global_mouse_up(s, cursor_x, cursor_y);
        }

        if (overlaps) {
            int local_x = cursor_x - s->screen_x;
            int local_y = cursor_y - s->screen_y;

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
    window_surface->screen_x = 50;
    window_surface->screen_y = 50;

    // Wallpaper.
    struct wallpaper_data *wallpaper_data = malloc(sizeof(*wallpaper_data));
    wallpaper_surface = surface_create(screen_width, screen_height,
                                       &wallpaper_ops, wallpaper_data);

    canvas_init(os->get_icon_png, os->open_asset_file);
}
