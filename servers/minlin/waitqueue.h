#ifndef __WAITQUEUE_H__
#define __WAITQUEUE_H__

#include <list.h>

struct proc;
struct waiter {
    list_elem_t next;
    struct proc *proc;
};

struct waitqueue {
    list_t queue;
};

void waitqueue_init(struct waitqueue *wq);
void waitqueue_add(struct waitqueue *wq, struct proc *proc);
void waitqueue_wake_one(struct waitqueue *wq);
void waitqueue_wake_all(struct waitqueue *wq);

#endif
