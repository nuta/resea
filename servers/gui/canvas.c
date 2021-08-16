#include "canvas.h"
#include <cairo-ft.h>
#include <cairo.h>
#include <ft2build.h>
#include <math.h>
#include <stdint.h>

#include FT_FREETYPE_H

#define ASSERT(expr)
#define MAX(a, b)                                                              \
    ({                                                                         \
        __typeof__(a) __a = (a);                                               \
        __typeof__(b) __b = (b);                                               \
        (__a > __b) ? __a : __b;                                               \
    })
#define MIN(a, b)                                                              \
    ({                                                                         \
        __typeof__(a) __a = (a);                                               \
        __typeof__(b) __b = (b);                                               \
        (__a < __b) ? __a : __b;                                               \
    })

struct canvas {
    cairo_t *cr;
    cairo_surface_t *surface;
};

static canvas_t canvas_create_from_surface(cairo_surface_t *surface) {
    cairo_t *cr = cairo_create(surface);
    ASSERT(dst->cr != NULL);

    struct canvas *canvas = malloc(sizeof(*canvas));
    canvas->cr = cr;
    canvas->surface = surface;
    return canvas;
}

canvas_t canvas_create(int width, int height) {
    cairo_surface_t *surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    ASSERT(surface != NULL);

    return canvas_create_from_surface(surface);
}

canvas_t canvas_create_from_buffer(int screen_width, int screen_height,
                                   void *framebuffer,
                                   enum canvas_format format) {
    cairo_format_t cairo_format;
    switch (format) {
        case CANVAS_FORMAT_ARGB32:
            cairo_format = CAIRO_FORMAT_ARGB32;
            break;
        default:
            // TODO: Return an error instead.
            cairo_format = CAIRO_FORMAT_RGB24;
    }

    int stride = cairo_format_stride_for_width(format, screen_width);
    cairo_surface_t *surface = cairo_image_surface_create_for_data(
        framebuffer, cairo_format, screen_width, screen_height, stride);
    ASSERT(surface != NULL);

    return canvas_create_from_surface(surface);
}

void canvas_draw_cursor(canvas_t canvas, enum cursor_shape shape) {
    cairo_set_source_rgb(canvas->cr, 255, 0, 0);
    cairo_rectangle(canvas->cr, 0, 0, 10, 10);
    cairo_fill(canvas->cr);
    cairo_surface_flush(canvas->surface);
}

void canvas_copy(canvas_t dst, canvas_t src, int x, int y) {
    int copy_width = MIN(cairo_image_surface_get_width(src->surface),
                         cairo_image_surface_get_width(dst->surface) - x);
    int copy_height = MIN(cairo_image_surface_get_height(src->surface),
                          cairo_image_surface_get_height(dst->surface) - y);

    cairo_translate(dst->cr, 0, 0);
    cairo_set_source_surface(dst->cr, src->surface, 0, 0);
    cairo_rectangle(dst->cr, x, y, copy_width, copy_height);
    cairo_fill(dst->cr);
}
