#include "internal.h"
#include <resea/ipc.h>

task_t ui_gui_server;

void ui_init(void) {
    ui_event_init();
    ui_gui_server = ipc_lookup("gui");
}
