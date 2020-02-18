#ifndef __SYS_H__
#define __SYS_H__

#include <list.h>
#include <types.h>
#include "tcp.h"

enum event_type {
    TCP_NEW_CLIENT,
    TCP_RECEIVED,
};

struct event {
    enum event_type type;
    list_elem_t next;
    union {
        struct {
            tcp_sock_t listen_sock;
        } tcp_new_client;
        struct {
            tcp_sock_t sock;
        } tcp_received;
    };
};

void sys_process_event(struct event *event);
msec_t sys_uptime(void);

#endif
