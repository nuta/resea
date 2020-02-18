#ifndef __MAIN_H__
#define __MAIN_H__

#define TIMER_INTERVAL 200

struct driver {
    list_elem_t next;
    tid_t tid;
    device_t device;
};

struct packet {
    list_elem_t next;
    tid_t dst;
    struct message m;
};

#endif
