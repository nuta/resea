#include "internal.h"
#include <resea/ipc.h>
#include <ui.h>

__failable ui_button_t ui_button_create(ui_window_t window, int x, int y,
                                        char *label) {
    struct message m;
    m.type = GUI_BUTTON_CREATE_MSG;
    m.gui_button_create.window = window;
    m.gui_button_create.label = label;
    m.gui_button_create.x = x;
    m.gui_button_create.y = y;
    error_t err = ipc_call(ui_gui_server, &m);
    if (err != OK) {
        return err;
    }

    return m.gui_button_create_reply.button;
}

error_t ui_button_set_label(ui_button_t button, char *label) {
    struct message m;
    m.type = GUI_BUTTON_SET_LABEL_MSG;
    m.gui_button_set_label.button = button;
    m.gui_button_set_label.label = label;
    return ipc_call(ui_gui_server, &m);
}

error_t ui_button_on_click(ui_button_t button,
                           void (*callback)(ui_button_t button, int x, int y)) {
    ui_register_event_callback(GUI_ON_BUTTON_CLICK_MSG, button, callback);
    return OK;
}
