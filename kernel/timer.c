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
        timer->current -= ticks;
        if (timer->current < 0) {
            timer->handler(timer);
            if (timer->interval) {
                timer->current = timer->interval;
            } else {
                timer_destroy(timer);
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

    struct timer *timer = KMALLOC(&small_arena, sizeof(struct timer));
    if (!timer) {
        table_free(&timers, timer_id);
        return NULL;
    }

    TRACE("timer_create: id=%d initial=%d, intrerval=%d",
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

void timer_destroy(struct timer *timer) {
    TRACE("timer_destroy: timer=%p", timer);
    table_free(&timers, timer->id);
    list_remove(&timer->next);
    // TODO: kfree(&small_arena, timer);
}

void timer_init(void) {
    list_init(&active_timers);
    if (table_init(&timers) != OK) {
        PANIC("failed to initialize timers");
    }
}
