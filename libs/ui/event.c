#include "internal.h"
#include <list.h>
#include <resea/async.h>
#include <resea/ipc.h>
#include <resea/malloc.h>

static list_t callbacks;

void ui_register_event_callback(msg_type_t msg_type, handle_t object,
                                __nonnull void *callback) {
    struct event_callback *ec = malloc(sizeof(*ec));
    ec->msg_type = msg_type;
    ec->object = object;
    ec->callback = callback;
    list_push_back(&callbacks, &ec->next);
}

static __nullable struct event_callback *get_callback(msg_type_t msg_type,
                                                      handle_t object) {
    LIST_FOR_EACH (ec, &callbacks, struct event_callback, next) {
        if (ec->object == object && ec->msg_type == msg_type) {
            return ec;
        }
    }

    return NULL;
}

void ui_handle_async(void) {
    struct message m;
    ASSERT_OK(async_recv(ui_gui_server, &m));

    switch (m.type) {
        case GUI_ON_BUTTON_CLICK_MSG: {
            struct event_callback *ec =
                get_callback(m.type, m.gui_on_button_click.button);
            if (ec) {
                ec->button_onclick_callback(m.gui_on_button_click.button,
                                            m.gui_on_button_click.x,
                                            m.gui_on_button_click.y);
            }
            break;
        }
        default:
            discard_unknown_message(&m);
    }
}

void ui_event_init(void) {
    list_init(&callbacks);
}
