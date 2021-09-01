#ifndef __UI_H__
#define __UI_H__

#include <types.h>

typedef handle_t ui_window_t;
__failable ui_window_t ui_window_create(const char *title, int width,
                                        int height);

typedef handle_t ui_text_t;
__failable ui_text_t ui_text_create(ui_window_t window);
error_t ui_text_set_position(ui_text_t text, int x, int y);
error_t ui_text_set_body(ui_text_t text, const char *body);
error_t ui_text_set_size(ui_text_t text, enum ui_text_size size);
error_t ui_text_set_rgba(ui_text_t text, uint8_t r, uint8_t g, uint8_t b,
                         uint8_t a);

typedef handle_t ui_button_t;
__failable ui_button_t ui_button_create(ui_window_t window, int x, int y,
                                        const char *label);
error_t ui_button_set_label(ui_button_t button, const char *label);
error_t ui_button_on_click(ui_button_t button,
                           void (*callback)(ui_button_t button, int x, int y));

#endif
