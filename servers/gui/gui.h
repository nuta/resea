#ifndef __GUI_H__
#define __GUI_H__

#include <list.h>
#include <types.h>

typedef void *backend_canvas_t;

struct canvas {
    backend_canvas_t backend_canvas;
};

struct window {};

struct surface {
    struct canvas canvas;
    int screen_x;
    int screen_y;
    list_elem_t next;
};

struct backend {
    backend_canvas_t (*get_back_buffer)(void);
    void (*swap_buffer)(void);
    backend_canvas_t (*create_framebuffer_canvas)(unsigned screen_width,
                                                  unsigned screen_height,
                                                  handle_t shm_handle);
    void (*copy_canvas)(backend_canvas_t dst, backend_canvas_t src, int x,
                        int y);
};

void gui_render(void);
void gui_init(struct backend *b);

#endif
