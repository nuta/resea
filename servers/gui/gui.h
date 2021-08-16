#ifndef __GUI_H__
#define __GUI_H__

#include "canvas.h"
#include <list.h>
#include <types.h>

struct surface_ops;
struct surface {
    list_elem_t next;
    struct surface_ops *ops;
    void *user_data;
    canvas_t canvas;
    int screen_x;
    int screen_y;
    int width;
    int height;
};

struct surface_ops {
    void (*render)(struct surface *surface);
};

struct os_ops {
    canvas_t (*get_back_buffer)(void);
    void (*swap_buffer)(void);
};

struct cursor_data {
    enum cursor_shape shape;
};

void gui_render(void);
void gui_init(struct os_ops *os_ops);

#endif
