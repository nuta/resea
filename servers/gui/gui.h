#ifndef __GUI_H__
#define __GUI_H__

#include "canvas.h"
#include <list.h>
#include <types.h>

struct surface_ops;
struct surface {
    list_elem_t next;
    struct surface_ops *ops;
    void *user_data;
    canvas_t canvas;
    int screen_x;
    int screen_y;
    int width;
    int height;
};

struct surface_ops {
    /// Called when the surface need to render the contents into its canvas.
    void (*render)(struct surface *surface);
    /// Called when the cursor is moved. `screen_x` and `screen_y` are global
    /// cursor position.
    void (*on_mouse_move)(int screen_x, int screen_y);
    /// Called when the cursor is on the surface. `x` and `y` are surface-local
    /// cursor position. If the callback returns `true`, it stop propagating
    void (*on_hover)(int x, int y);
    /// Called on a mouse click. `x` and `y` are surface-local cursor position.
    /// If the callback returns `true`, gui stops propagating the clicked event
    /// to remaining surfaces behind it (like `Event.stopPropagation` in Web).
    bool (*on_clicked_left)(int x, int y);
};

struct os_ops {
    canvas_t (*get_back_buffer)(void);
    void (*swap_buffer)(void);
    void *(*get_icon_png)(enum icon_type icon, unsigned *file_size);
    void *(*open_asset_file)(const char *name, unsigned *file_size);
};

struct cursor_data {
    enum cursor_shape shape;
};

struct wallpaper_data {};

void gui_render(void);
void gui_move_mouse(int x_delta, int y_delta, bool clicked_left,
                    bool clicked_right);
void gui_init(int screen_width_, int screen_height_, struct os_ops *os_);

#endif
