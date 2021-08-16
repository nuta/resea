#include "canvas.h"

struct canvas {};

canvas_t canvas_create(int width, int height) {
    struct canvas *canvas = malloc(sizeof(*canvas));
    return canvas;
}

canvas_t canvas_create_from_buffer(int screen_width, int screen_height,
                                   void *framebuffer) {
    struct canvas *canvas = malloc(sizeof(*canvas));
    return canvas;
}

void canvas_draw_cursor(canvas_t canvas, enum cursor_shape shape) {
}

void canvas_copy(canvas_t dst, canvas_t src, int x, int y) {
}
