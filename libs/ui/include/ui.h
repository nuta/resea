#ifndef __UI_H__
#define __UI_H__

#include <types.h>

typedef int ui_event_t;

typedef handle_t ui_text_t;
ui_text_t ui_text_create(void);
void ui_text_set_body(ui_text_t text, const char *body);
void ui_text_set_size(ui_text_t text, enum ui_text_size size);
void ui_text_set_rgba(ui_text_t text, uint8_t r, uint8_t g, uint8_t b,
                      uint8_t a);

typedef handle_t ui_button_t;
ui_button_t ui_button_create(void);
void ui_button_set_text(ui_button_t button, ui_text_t text);
void ui_button_on_click(ui_button_t button,
                        void (*callback)(ui_event_t ev, ui_button_t button));

typedef handle_t ui_surface_t;
void ui_draw_text(ui_surface_t surface, ui_text_t text, int x, int y);
void ui_draw_button(ui_surface_t surface, ui_button_t button, int x, int y);

typedef handle_t ui_window_t;
ui_window_t ui_window_create(void);
ui_surface_t ui_window_get_surface(void);
void ui_window_set_size(ui_window_t win, int width, int height);
void ui_window_set_title(ui_window_t win, const char *title);

#endif
