#include "gui.h"
#include <resea/malloc.h>

static struct os_ops *os = NULL;

/// The list of surfaces ordered by the Z index. The frontmost one (i.e. cursor)
/// comes first.
static list_t surfaces;
struct surface *cursor_surface = NULL;
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

void cursor_render(struct surface *surface) {
    struct cursor_data *data = surface->user_data;
    canvas_draw_cursor(surface->canvas, data->shape);
}

static struct surface_ops cursor_ops = {
    .render = cursor_render,
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
    struct cursor_data *data = cursor_surface->user_data;
    cursor_surface->screen_x =
        MIN(MAX(0, cursor_surface->screen_x + x_delta), screen_width - 5);
    cursor_surface->screen_y =
        MIN(MAX(0, cursor_surface->screen_y + y_delta), screen_height - 5);
}

void gui_init(int screen_width_, int screen_height_, struct os_ops *os_) {
    os = os_;
    screen_width = screen_width_;
    screen_height = screen_height_;
    list_init(&surfaces);

    // Initialize the mouse cursor.
    struct cursor_data *cursor_data = malloc(sizeof(*cursor_data));
    cursor_data->shape = CURSOR_POINTER;
    cursor_surface = surface_create(48, 48, &cursor_ops, cursor_data);

    canvas_init(os->get_icon_png);
}
