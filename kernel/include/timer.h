#ifndef __TIMER_H__
#define __TIMER_H__

#include <types.h>
#include <list.h>

#define TICK_HZ 1000
#define THREAD_SWITCH_INTERVAL ((20 * 1000) / TICK_HZ)

enum timer_type {
    TIMER_TIMEOUT,
    TIMER_INTERVAL,
};

struct timer {
    struct list_head next;
    int id;
    int current;
    int interval;
    void (*handler)(struct timer *);
    void *arg;
};

void timer_interrupt_handler(int ticks);
struct timer *timer_create(enum timer_type type, int msec,
                           void (*handler)(struct timer *), void *arg);
void timer_destroy(struct timer *timer);
void timer_init(void);

#endif
