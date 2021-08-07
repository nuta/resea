#include <cairo-ft.h>
#include <cairo.h>
#include <ft2build.h>
#include <math.h>
#include <stdint.h>

#include FT_FREETYPE_H

static cairo_surface_t *surface = NULL;
uint32_t *image = NULL;
FT_Library ft_lib;
FT_Face ft_face;
cairo_font_face_t *font_face = NULL;

static void draw_rounded_rectangle(cairo_t *cr, double x, double y,
                                   double width, double height) {
    /* a custom shape that could be wrapped in a function */

    double aspect = 20.0;                /* aspect ratio */
    double corner_radius = width / 10.0; /* and corner curvature radius */

    double radius = corner_radius / aspect;
    double degrees = M_PI / 180.0;

    cairo_new_sub_path(cr);
    cairo_arc(cr, x + width - radius, y + radius, radius, -90 * degrees,
              0 * degrees);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0 * degrees,
              90 * degrees);
    cairo_arc(cr, x + radius, y + height - radius, radius, 90 * degrees,
              180 * degrees);
    cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);

    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, .7);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.);
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);
}

static void draw_infobar(cairo_t *cr, double screen_width, double screen_height,
                         double x, double y, double margin_bottom) {
    double width = screen_width - 2 * x;
    double height = 20.;
    draw_rounded_rectangle(cr, x, y, width, height);

    cairo_set_font_face(cr, font_face);
    cairo_set_font_size(cr, 15);
    cairo_set_source_rgb(cr, 1., 1., 1.);
    cairo_move_to(cr, screen_width - 130., y + 15.);
    cairo_show_text(cr, "Sat Aug 7 17:00");
}

static void draw_console(cairo_t *cr, double screen_width, double screen_height,
                         double x, double y, double margin_bottom) {
    double width = screen_width - 2 * x;
    double height = screen_height - y - margin_bottom;
    draw_rounded_rectangle(cr, x, y, width, height);

    cairo_set_font_face(cr, font_face);
    cairo_set_font_size(cr, 17);
    cairo_set_source_rgb(cr, 1., 1., 1.);
    cairo_move_to(cr, x + 10., y + 30.);
    cairo_show_text(cr, "$ list servers");
}

struct read_func_closure {
    uint8_t *file_data;
    size_t file_size;
    unsigned offset;
};

static cairo_status_t embedded_read_func(void *closure, unsigned char *data,
                                         unsigned int len) {
    struct read_func_closure *c = (struct read_func_closure *) closure;
    if (c->offset + len > c->file_size) {
        printf("embedded_read_func: read beyond size\n");
        return CAIRO_STATUS_READ_ERROR;
    }

    printf("read wallpaper: off=%x, len=%d\n", c->offset, len);
    memcpy(data, &c->file_data[c->offset], len);
    c->offset += len;
    return CAIRO_STATUS_SUCCESS;
}

extern char _binary_servers_drivers_gpu_virtio_gpu_wallpaper_png_start[];
extern char _binary_servers_drivers_gpu_virtio_gpu_wallpaper_png_size[];
cairo_surface_t *wallpaper_surface = NULL;

static void draw_wallpaper(cairo_t *cr, double screen_width,
                           double screen_height) {
    cairo_translate(cr, 0, 0);
    cairo_set_source_surface(cr, wallpaper_surface, 0, 0);
    cairo_rectangle(cr, 0, 0, screen_width, screen_height);
    cairo_fill(cr);
}

static void draw(uint32_t width, uint32_t height) {
    printf("draw(): %d\n", __LINE__);
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    printf("draw(): %d\n", __LINE__);
    cairo_t *cr = cairo_create(surface);
    printf("draw(): %d\n", __LINE__);
    printf("surface = %p, cairo_status = %d (success =%d)\n", surface,
           cairo_status(cr), CAIRO_STATUS_SUCCESS);

    // cairo_pattern_t *pat;

    // pat = cairo_pattern_create_linear(0.0, 0.0, 0.0, 256.0);
    // cairo_pattern_add_color_stop_rgba(pat, 1, 0, 0, 0, 1);
    // cairo_pattern_add_color_stop_rgba(pat, 0, 1, 1, 1, 1);
    // cairo_rectangle(cr, 0, 0, 256, 256);
    // cairo_set_source(cr, pat);
    // cairo_fill(cr);
    // cairo_pattern_destroy(pat);

    // pat = cairo_pattern_create_radial(115.2, 102.4, 25.6, 102.4, 102.4,
    // 128.0); cairo_pattern_add_color_stop_rgba(pat, 0, 0, 1, 1, 1);
    // cairo_pattern_add_color_stop_rgba(pat, 1, 0, 0, 0, 1);
    // cairo_set_source(cr, pat);
    // cairo_arc(cr, 128.0, 128.0, 76.8, 0, 2 * M_PI);
    // cairo_fill(cr);
    // cairo_pattern_destroy(pat);

    cairo_set_source_rgb(cr, 255, 255, 255);
    cairo_rectangle(cr, 00, 0, width, height);
    cairo_fill(cr);

    // cairo_set_font_face(cr, font_face);
    // cairo_set_font_size(cr, 70);
    // cairo_set_source_rgb(cr, 0., 0., 0.);
    // cairo_move_to(cr, 120, 120);
    // cairo_show_text(cr, "Hello World!");

    draw_wallpaper(cr, width, height);
    draw_infobar(cr, width, height, 15., 5., 15.);
    draw_console(cr, width, height, 15., 45., 15.);

    cairo_surface_flush(surface);

    image = (uint32_t *) cairo_image_surface_get_data(surface);
    printf("surface = %p, cairo_status = %d (success =%d)\n", surface,
           cairo_status(cr), CAIRO_STATUS_SUCCESS);
    printf("p = %p\n", image);
}

extern char
    _binary_build_servers_drivers_gpu_virtio_gpu_Roboto_Regular_ttf_start[];
extern char
    _binary_build_servers_drivers_gpu_virtio_gpu_Roboto_Regular_ttf_size[];

void cairo_demo(uint32_t *framebuffer, uint32_t width, uint32_t height) {
    static int inited = 0;
    if (!inited) {
        printf(
            "font size: %dKB\n",
            ((long)
                 _binary_build_servers_drivers_gpu_virtio_gpu_Roboto_Regular_ttf_size)
                / 1024);
        printf("FT_init: %d\n", __LINE__);
        FT_Init_FreeType(&ft_lib);
        printf("FT_new: %d\n", __LINE__);
        FT_New_Memory_Face(
            ft_lib,
            (const unsigned char *)
                _binary_build_servers_drivers_gpu_virtio_gpu_Roboto_Regular_ttf_start,
            (long)
                _binary_build_servers_drivers_gpu_virtio_gpu_Roboto_Regular_ttf_size,
            0, &ft_face);

        font_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
        static const cairo_user_data_key_t key;
        cairo_status_t status = cairo_font_face_set_user_data(
            font_face, &key, ft_face, (cairo_destroy_func_t) FT_Done_Face);

        struct read_func_closure closure = {
            .file_data =
                _binary_servers_drivers_gpu_virtio_gpu_wallpaper_png_start,
            .file_size = (long)
                _binary_servers_drivers_gpu_virtio_gpu_wallpaper_png_size,
            .offset = 0};

        wallpaper_surface = cairo_image_surface_create_from_png_stream(
            embedded_read_func, &closure);

        draw(width, height);
        inited = 1;
    }

    if (!image) {
        printf("failed to draw a image by cairo\n");
        return;
    }

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t pixel = image[y * width + x];
            uint32_t r = (pixel >> 16) & 0xff;
            uint32_t g = (pixel >> 8) & 0xff;
            uint32_t b = pixel & 0xff;
            framebuffer[y * width + x] = (b << 16) | (g << 8) | r;
        }
    }
}
