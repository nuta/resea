#ifndef __GUI_H__
#define __GUI_H__

#include <list.h>
#include <types.h>

struct canvas_ops;
struct canvas {
    struct canvas_ops *ops;
    void *user_data;
};

struct canvas_ops {
    void (*copy_from_other)(struct canvas *canvas, struct canvas *src, int x,
                            int y);
};

struct surface_ops;
struct surface {
    list_elem_t next;
    struct surface_ops *ops;
    void *user_data;
    struct canvas *canvas;
    int screen_x;
    int screen_y;
    int width;
    int height;
};

struct surface_ops {
    void (*render)(struct surface *surface);
};

struct os_ops {
    struct canvas *(*get_back_buffer)(void);
    void (*swap_buffer)(void);
    struct canvas *(*create_canvas)(int width, int height);
    struct canvas *(*create_framebuffer_canvas)(int screen_width,
                                                int screen_height,
                                                handle_t shm_handle);
};

enum cursor_shape {
    CURSOR_POINTER,
};

struct cursor_data {
    enum cursor_shape shape;
};

void gui_render(void);
void gui_init(struct os_ops *os_ops);

#endif
