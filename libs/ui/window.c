#include "internal.h"
#include <resea/ipc.h>
#include <ui.h>

__failable ui_window_t ui_window_create(const char *title, int width,
                                        int height) {
    struct message m;
    m.type = GUI_WINDOW_CREATE_MSG;
    m.gui_window_create.title = title;
    m.gui_window_create.width = width;
    m.gui_window_create.height = height;
    error_t err = ipc_call(ui_gui_server, &m);
    if (err != OK) {
        return err;
    }

    return m.gui_window_create_reply.window;
}
