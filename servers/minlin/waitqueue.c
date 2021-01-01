#include "waitqueue.h"
#include "proc.h"
#include <resea/malloc.h>
#include <string.h>

void waitqueue_init(struct waitqueue *wq) {
    list_init(&wq->queue);
}

void waitqueue_add(struct waitqueue *wq, struct proc *proc) {
    struct waiter *w = malloc(sizeof(*w));
    w->proc = proc;
    list_push_back(&wq->queue, &w->next);
}

void waitqueue_wake_one(struct waitqueue *wq) {
    if (list_is_empty(&wq->queue)) {
        return;
    }

    struct waiter *w = LIST_POP_FRONT(&wq->queue, struct waiter, next);
    proc_resume(w->proc);
    free(w);
}

void waitqueue_wake_all(struct waitqueue *wq) {
    // FIXME:
    //    while (!list_is_empty(&old_queue)) {
    waitqueue_wake_one(wq);
    //    }
}
