#ifndef __UI_H__
#define __UI_H__

#include <types.h>

typedef int ui_event_t;

typedef handle_t ui_text_t;
__failable ui_text_t ui_text_create(void);
error_t ui_text_set_visibility(ui_text_t text, bool visible);
error_t ui_text_set_position(ui_text_t text, int x, int y);
error_t ui_text_set_body(ui_text_t text, const char *body);
error_t ui_text_set_size(ui_text_t text, enum ui_text_size size);
error_t ui_text_set_rgba(ui_text_t text, uint8_t r, uint8_t g, uint8_t b,
                         uint8_t a);

typedef handle_t ui_button_t;
__failable ui_button_t ui_button_create(void);
error_t ui_buttton_set_visibility(ui_button_t button, bool visible);
error_t ui_buttton_set_position(ui_button_t button, int x, int y);
error_t ui_buttton_set_size(ui_button_t button, int x, int y);
error_t ui_button_set_text(ui_button_t button, ui_text_t text);
error_t ui_button_on_click(ui_button_t button,
                           void (*callback)(ui_event_t ev, ui_button_t button));

typedef handle_t ui_window_t;
__failable ui_window_t ui_window_create(void);
error_t ui_window_set_size(ui_window_t window, int width, int height);
error_t ui_window_set_title(ui_window_t window, const char *title);
error_t ui_window_add_text(ui_window_t window, ui_text_t text);
error_t ui_window_add_button(ui_window_t window, ui_button_t button);

#endif
