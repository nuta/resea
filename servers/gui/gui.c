#include "gui.h"

static struct os_ops *os = NULL;

/// The list of surfaces ordered by the Z index. The frontmost one (i.e. cursor)
/// comes first.
static list_t surfaces;

void gui_render(void) {
    struct canvas *screen = os->get_back_buffer();
    LIST_FOR_EACH (s, &surfaces, struct surface, next) {
        s->ops->render(s);
        os->copy_canvas(screen, s->canvas, s->screen_x, s->screen_y);
    }

    os->swap_buffer();
}

void gui_init(struct os_ops *os_ops) {
    os = os_ops;
    list_init(&surfaces);
}
