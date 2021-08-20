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
    .on_mouse_move = NULL,
    .on_hover = NULL,
    .on_clicked_left = NULL,
};

static void wallpaper_render(struct surface *surface) {
    struct wallpaper_data *data = surface->user_data;
    canvas_draw_wallpaper(surface->canvas);
}

static struct surface_ops wallpaper_ops = {
    .render = wallpaper_render,
    .on_mouse_move = NULL,
    .on_hover = NULL,
    .on_clicked_left = NULL,
};

static void window_mouse_move(int screen_x, int screen_y) {
}

static bool window_clicked_left(int x, int y) {
    return true;
}

static void window_render(struct surface *surface) {
    struct window_data *data = surface->user_data;
    canvas_draw_window(surface->canvas, data);
}

static struct surface_ops window_ops = {
    .render = window_render,
    .on_mouse_move = window_mouse_move,
    .on_hover = NULL,
    .on_clicked_left = window_clicked_left,
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
    bool clicked_left_event = !clicked_left && prev_clicked_left;
    bool consumed_clicked_left = false;
    LIST_FOR_EACH (s, &surfaces, struct surface, next) {
        bool overlaps =
            s->screen_x <= cursor_x && cursor_x < s->screen_x + s->width
            && s->screen_y <= cursor_y && cursor_y < s->screen_y + s->height;

        if (overlaps) {
            int local_x = s->screen_x - cursor_x;
            int local_y = s->screen_y - cursor_y;

            if (s->ops->on_hover) {
                s->ops->on_hover(local_x, local_y);
            }

            if (clicked_left_event && !consumed_clicked_left
                && s->ops->on_clicked_left) {
                consumed_clicked_left =
                    s->ops->on_clicked_left(local_x, local_y);
            }
        }
    }
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
    window_surface->screen_x = 50;
    window_surface->screen_y = 50;

    // Wallpaper.
    struct wallpaper_data *wallpaper_data = malloc(sizeof(*wallpaper_data));
    wallpaper_surface = surface_create(screen_width, screen_height,
                                       &wallpaper_ops, wallpaper_data);

    canvas_init(os->get_icon_png, os->open_asset_file);
}
