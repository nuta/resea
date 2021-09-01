#ifndef __UI_INTERNAL_H__
#define __UI_INTERNAL_H__

#include <list.h>
#include <types.h>

extern task_t ui_gui_server;

struct event_callback {
    list_elem_t next;
    /// The acceptable event type such as `GUI_ON_BUTTON_CLICK_MSG`.
    msg_type_t msg_type;
    /// A gui's object such as button.
    handle_t object;
    /// The callback handler for the event.
    union {
        __nonnull void *callback;
        (*button_onclick_callback)(handle_t button, int x, int y);
    };
};

void ui_register_event_callback(msg_type_t msg_type, handle_t object,
                                __nonnull void *callback);
void ui_handle_event(void);
void ui_event_init(void);

#endif
