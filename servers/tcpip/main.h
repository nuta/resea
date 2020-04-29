#ifndef __MAIN_H__
#define __MAIN_H__

#include <types.h>

#define TIMER_INTERVAL 200

struct driver {
    list_elem_t next;
    task_t tid;
    device_t device;
    list_t tx_queue;
    msec_t last_dhcp_discover;
    int dhcp_discover_retires;
};

struct packet {
    list_elem_t next;
    task_t dst;
    struct message m;
};

#endif
