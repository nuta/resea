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
    LIST_FOR_EACH(timer, &active_timers, struct timer, next) {
        if (timer->current <= 0) {
            // The timer is disabled or is an already expired oneshot one.
            continue;
        }

        timer->current -= ticks;
        if (timer->current <= 0) {
            timer->handler(timer);
            if (timer->interval) {
                timer->current = timer->interval;
            }
        }
    }
}

struct timer *timer_create(int initial, int interval,
                           void (*handler)(struct timer *), void *arg) {
    int timer_id = table_alloc(&timers);
    if (!timer_id) {
        return NULL;
    }

    struct timer *timer = KMALLOC(&object_arena, sizeof(struct timer));
    if (!timer) {
        table_free(&timers, timer_id);
        return NULL;
    }

    TRACE("timer_create: id=%d initial=%d, interval=%d",
          timer_id, initial, interval);
    timer->id = timer_id;
    timer->current = initial;
    timer->interval = interval;
    timer->handler = handler;
    timer->arg = arg;
    list_push_back(&active_timers, &timer->next);
    table_set(&timers, timer_id, timer);
    return timer;
}

void timer_reset(struct timer *timer, int initial, int interval) {
    TRACE("timer_reset: id=%d initial=%d, interval=%d",
          timer->id, initial, interval);
    timer->interval = interval;
    timer->current = initial;
}

void timer_destroy(struct timer *timer) {
    TRACE("timer_destroy: timer=%p", timer);
    table_free(&timers, timer->id);
    list_remove(&timer->next);
    kfree(&object_arena, timer);
}

void timer_init(void) {
    list_init(&active_timers);
    if (table_init(&timers) != OK) {
        PANIC("failed to initialize timers");
    }
}
