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

static cairo_surface_t *icons[NUM_ICON_TYPES];

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
    cairo_set_source_surface(canvas->cr, icons[ICON_POINTER], 0, 0);
    cairo_rectangle(canvas->cr, 0, 0, 48, 48);
    cairo_surface_flush(canvas->surface);
}

void canvas_copy(canvas_t dst, canvas_t src, int x, int y) {
    int copy_width = MIN(cairo_image_surface_get_width(src->surface),
                         cairo_image_surface_get_width(dst->surface) - x);
    int copy_height = MIN(cairo_image_surface_get_height(src->surface),
                          cairo_image_surface_get_height(dst->surface) - y);

    cairo_set_source_surface(dst->cr, src->surface, x, y);
    cairo_rectangle(dst->cr, x, y, copy_width, copy_height);
    cairo_fill(dst->cr);
    cairo_surface_flush(dst->surface);
}

struct read_func_closure {
    uint8_t *file_data;
    unsigned file_size;
    unsigned offset;
};

static cairo_status_t embedded_read_func(void *closure, unsigned char *data,
                                         unsigned int len) {
    struct read_func_closure *c = (struct read_func_closure *) closure;
    if (c->offset + len > c->file_size) {
        return CAIRO_STATUS_READ_ERROR;
    }

    memcpy(data, &c->file_data[c->offset], len);
    c->offset += len;
    return CAIRO_STATUS_SUCCESS;
}

void canvas_init(void *(*get_icon_png)(enum icon_type icon,
                                       unsigned *file_size)) {
    for (int type = 0; type < NUM_ICON_TYPES; type++) {
        unsigned file_size;
        uint8_t *file_data = get_icon_png(type, &file_size);
        struct read_func_closure closure = {
            .file_data = file_data,
            .file_size = file_size,
            .offset = 0,
        };

        icons[type] = cairo_image_surface_create_from_png_stream(
            embedded_read_func, &closure);
    }
}
