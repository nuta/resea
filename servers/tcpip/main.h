#ifndef __MAIN_H__
#define __MAIN_H__

#define TIMER_INTERVAL 200

struct driver {
    list_elem_t next;
    task_t tid;
    device_t device;
    list_t tx_queue;
};

struct packet {
    list_elem_t next;
    task_t dst;
    struct message m;
};

#endif
