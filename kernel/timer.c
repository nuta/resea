#include <types.h>
#include <thread.h>
#include <memory.h>
#include <timer.h>
#include <table.h>

/// The timers table.
static struct table timers;
static struct list_head active_timers;

/// The timer interrupt handler.
void timer_interrupt_handler(int ticks) {
    LIST_FOR_EACH(e, &active_timers) {
        struct timer *timer = LIST_CONTAINER(timer, next, e);
        timer->current -= ticks;
        if (timer->current < 0) {
            timer->handler(timer);
            if (timer->interval) {
                timer->current = timer->interval;
            } else {
                // TODO: foreach_destroyable
                timer_destroy(timer);
            }
        }
    }
}

struct timer *timer_create(enum timer_type type, int msec,
                           void (*handler)(struct timer *), void *arg) {
    int timer_id = table_alloc(&timers);
    if (!timer_id) {
        return NULL;
    }

    struct timer *timer = kmalloc(&small_arena);
    if (!timer) {
        table_free(&timers, timer_id);
        return NULL;
    }

    TRACE("timer_create: type=%d, msec=%d", type, msec, timer_id);
    timer->id = timer_id;
    timer->current = msec;
    timer->interval = (type == TIMER_INTERVAL) ? msec : 0;
    timer->handler = handler;
    timer->arg = arg;
    list_push_back(&active_timers, &timer->next);
    table_set(&timers, timer_id, timer);
    return timer;
}

void timer_destroy(struct timer *timer) {
    TRACE("timer_destroy: timer=%p", timer);
    table_free(&timers, timer->id);
    // TODO: list_remove(&active_timers, &timer->next);
    // TODO: kfree(&small_arena, timer);
}

void timer_init(void) {
    list_init(&active_timers);
    if (table_init(&timers) != OK) {
        PANIC("failed to initialize timers");
    }
}
