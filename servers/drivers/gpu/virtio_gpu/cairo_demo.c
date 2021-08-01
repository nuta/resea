#include <cairo.h>
#include <math.h>
#include <stdint.h>

static cairo_surface_t *surface = NULL;
uint32_t *image = NULL;

static void draw(uint32_t width, uint32_t height) {
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cr = cairo_create(surface);
    printf("surface = %p, cairo_status = %d (success =%d)\n", surface,
           cairo_status(cr), CAIRO_STATUS_SUCCESS);

    cairo_pattern_t *pat;

    pat = cairo_pattern_create_linear(0.0, 0.0, 0.0, 256.0);
    cairo_pattern_add_color_stop_rgba(pat, 1, 0, 0, 0, 1);
    cairo_pattern_add_color_stop_rgba(pat, 0, 1, 1, 1, 1);
    cairo_rectangle(cr, 0, 0, 256, 256);
    cairo_set_source(cr, pat);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);

    pat = cairo_pattern_create_radial(115.2, 102.4, 25.6, 102.4, 102.4, 128.0);
    cairo_pattern_add_color_stop_rgba(pat, 0, 0, 1, 1, 1);
    cairo_pattern_add_color_stop_rgba(pat, 1, 0, 0, 0, 1);
    cairo_set_source(cr, pat);
    cairo_arc(cr, 128.0, 128.0, 76.8, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);

    // cairo_set_source_rgb(cr, 80, 255, 255);
    // cairo_rectangle(cr, 50, 50, 400, 400);
    // cairo_fill(cr);

    cairo_surface_flush(surface);

    image = cairo_image_surface_get_data(surface);
    printf("surface = %p, cairo_status = %d (success =%d)\n", surface,
           cairo_status(cr), CAIRO_STATUS_SUCCESS);
    printf("p = %p\n", image);
}

void cairo_demo(uint32_t *framebuffer, uint32_t width, uint32_t height) {
    static int drawed = 0;
    if (!drawed) {
        draw(width, height);
        drawed = 1;
    }

    if (!image) {
        printf("failed to draw a image by cairo\n");
        return;
    }

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t pixel = image[y * width + x];
            framebuffer[y * width + x] = pixel;
        }
    }
}
