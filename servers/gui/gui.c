#include "gui.h"

static list_t surfaces;
static struct backend *backend = NULL;

static void render_surface(struct surface *s) {
}

void gui_render(void) {
    backend_canvas_t buffer = backend->get_back_buffer();
    LIST_FOR_EACH (s, &surfaces, struct surface, next) {
        render_surface(s);
        backend->copy_canvas(buffer, s->canvas.backend_canvas, s->screen_x,
                             s->screen_y);
    }

    backend->swap_buffer();
}

void gui_init(struct backend *b) {
    backend = b;
    list_init(&surfaces);
}
