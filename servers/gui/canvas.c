#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// canvas.h depends on stdint
#include "canvas.h"
#include <cairo-ft.h>
#include <cairo.h>
#include <ft2build.h>
#include <math.h>

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
static FT_Library ft_lib;
static FT_Face ft_ui_regular_face;
static FT_Face ft_ui_bold_face;
static cairo_font_face_t *ui_regular_font = NULL;
static cairo_font_face_t *ui_bold_font = NULL;

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

void canvas_draw_wallpaper(canvas_t canvas) {
    cairo_set_source_rgb(canvas->cr, .8, .6, .5);
    cairo_rectangle(canvas->cr, 0, 0,
                    cairo_image_surface_get_width(canvas->surface),
                    cairo_image_surface_get_height(canvas->surface));
    cairo_fill(canvas->cr);
}

void canvas_draw_window(canvas_t canvas, struct window_data *window) {
    int width = cairo_image_surface_get_width(canvas->surface) - 3;
    int height = cairo_image_surface_get_height(canvas->surface) - 3;

    // Window frame.
    double radius = 10.;
    double degrees = M_PI / 180.;
    cairo_new_sub_path(canvas->cr);
    cairo_arc(canvas->cr, width - radius, radius, radius, -90 * degrees,
              0 * degrees);
    cairo_arc(canvas->cr, width - radius, height - radius, radius, 0 * degrees,
              90 * degrees);
    cairo_arc(canvas->cr, radius, height - radius, radius, 90 * degrees,
              180 * degrees);
    cairo_arc(canvas->cr, radius, radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(canvas->cr);

    cairo_set_source_rgb(canvas->cr, .97, .97, .97);
    cairo_fill(canvas->cr);

    // Window shadow.
    cairo_arc(canvas->cr, width - radius, radius, radius, -90 * degrees,
              0 * degrees);
    cairo_arc(canvas->cr, width - radius, height - radius, radius, 0 * degrees,
              90 * degrees);
    cairo_arc(canvas->cr, radius, height - radius, radius, 90 * degrees,
              180 * degrees);
    cairo_set_line_width(canvas->cr, 3);
    cairo_set_source_rgb(canvas->cr, .55, .55, .55);
    cairo_stroke(canvas->cr);

    // Title bar background.
    cairo_arc(canvas->cr, width - radius, radius, radius, -90 * degrees,
              0 * degrees);
    cairo_line_to(canvas->cr, width, WINDOW_TITLE_HEIGHT);
    cairo_line_to(canvas->cr, 0, WINDOW_TITLE_HEIGHT);
    cairo_arc(canvas->cr, radius, radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(canvas->cr);
    cairo_set_source_rgb(canvas->cr, .8, .8, .8);
    cairo_fill(canvas->cr);

    // Close button.
    cairo_arc(canvas->cr, 15, 11, 7, 0, 2 * M_PI);
    cairo_set_source_rgb(canvas->cr, .3, .3, .95);
    cairo_fill(canvas->cr);

    // Window title.
    const char *title = "Console";
    cairo_text_extents_t extents;
    cairo_set_font_face(canvas->cr, ui_bold_font);
    cairo_set_font_size(canvas->cr, WINDOW_TITLE_HEIGHT - 10);
    cairo_text_extents(canvas->cr, title, &extents);
    cairo_set_source_rgb(canvas->cr, .1, .1, .1);
    cairo_move_to(canvas->cr, width / 2 - extents.width / 2,
                  extents.height + 7);
    cairo_show_text(canvas->cr, title);
}

void canvas_draw_cursor(canvas_t canvas, enum cursor_shape shape) {
    cairo_set_source_surface(canvas->cr, icons[ICON_CURSOR], 0, 0);
    cairo_rectangle(canvas->cr, 0, 0, ICON_SIZE, ICON_SIZE);
    cairo_fill(canvas->cr);
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
                                       unsigned *file_size),
                 void *(*open_asset_file)(const char *name,
                                          unsigned *file_size)) {
    unsigned font_size;
    uint8_t *font_file;

    FT_Init_FreeType(&ft_lib);

    // Open a font file.
    font_file = open_asset_file("ui-regular.ttf", &font_size);
    FT_New_Memory_Face(ft_lib, font_file, font_size, 0, &ft_ui_regular_face);
    ui_regular_font =
        cairo_ft_font_face_create_for_ft_face(ft_ui_regular_face, 0);

    // Open a font file.
    font_file = open_asset_file("ui-bold.ttf", &font_size);
    FT_New_Memory_Face(ft_lib, font_file, font_size, 0, &ft_ui_bold_face);
    ui_bold_font = cairo_ft_font_face_create_for_ft_face(ft_ui_bold_face, 0);

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

        printf("cursor size: %dx%d [%x, %x, %x, %x]\n",
               cairo_image_surface_get_width(icons[ICON_CURSOR]),
               cairo_image_surface_get_height(icons[ICON_CURSOR]), file_data[0],
               file_data[1], file_data[2], file_data[3]);
    }
}
