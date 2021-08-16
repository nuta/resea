#ifndef __RENDERER_H__
#define __RENDERER_H__

enum cursor_shape {
    CURSOR_POINTER,
};

typedef void *canvas_t;

canvas_t canvas_create(int width, int height);
canvas_t canvas_create_from_buffer(int screen_width, int screen_height,
                                   void *framebuffer);
void canvas_draw_cursor(void *);
void canvas_copy(canvas_t dst, canvas_t src, int x, int y);

#endif
