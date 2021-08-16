#include "gui.h"
#include <resea/malloc.h>

static struct os_ops *os = NULL;

/// The list of surfaces ordered by the Z index. The frontmost one (i.e. cursor)
/// comes first.
static list_t surfaces;
struct surface *cursor_surface = NULL;

static struct surface *surface_create(int width, int height,
                                      struct surface_ops *ops,
                                      void *user_data) {
    struct surface *surface = malloc(sizeof(*surface));
    surface->canvas = os->create_canvas(width, height);
    surface->ops = ops;
    surface->user_data = user_data;
    surface->screen_x = 0;
    surface->screen_y = 0;
    surface->width = width;
    surface->height = height;
    return surface;
}

void cursor_render(struct surface *surface) {
}

static struct surface_ops cursor_ops = {
    .render = cursor_render,
};

void gui_render(void) {
    struct canvas *screen = os->get_back_buffer();
    LIST_FOR_EACH_REV (s, &surfaces, struct surface, next) {
        s->ops->render(s);
        screen->ops->copy_from_other(screen, s->canvas, s->screen_x,
                                     s->screen_y);
    }

    os->swap_buffer();
}

void gui_init(struct os_ops *os_ops) {
    os = os_ops;
    list_init(&surfaces);

    struct cursor_data *cursor_data = malloc(sizeof(*cursor_data));
    cursor_data->shape = CURSOR_POINTER;
    cursor_surface = surface_create(15, 15, &cursor_ops, cursor_data);
}
