#ifndef __RENDERER_H__
#define __RENDERER_H__

enum canvas_format {
    CANVAS_FORMAT_ARGB32,
};

enum cursor_shape {
    CURSOR_POINTER,
};

enum icon_type {
    ICON_POINTER = 0,
    NUM_ICON_TYPES = 1,
};

struct window_data {};

struct canvas;
typedef struct canvas *canvas_t;

canvas_t canvas_create(int width, int height);
canvas_t canvas_create_from_buffer(int screen_width, int screen_height,
                                   void *framebuffer,
                                   enum canvas_format format);
void canvas_draw_wallpaper(canvas_t canvas);
void canvas_draw_window(canvas_t canvas, struct window_data *window);
void canvas_draw_cursor(canvas_t canvas, enum cursor_shape shape);
void canvas_copy(canvas_t dst, canvas_t src, int x, int y);
void canvas_init(void *(*get_icon_png)(enum icon_type icon,
                                       unsigned *file_size),
                 void *(*open_asset_file)(const char *name,
                                          unsigned *file_size));

#endif
