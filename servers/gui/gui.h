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
void gui_move_mouse(int x_delta, int y_delta, bool clicked_left,
                    bool clicked_right);
void gui_init(int screen_width_, int screen_height_, struct os_ops *os_);

#endif
