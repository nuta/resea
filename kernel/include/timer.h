#ifndef __TIMER_H__
#define __TIMER_H__

#include <types.h>
#include <list.h>

#define TICK_HZ 1000
#define THREAD_SWITCH_INTERVAL ((20 * 1000) / TICK_HZ)

struct timer {
    struct list_head next;
    int id;
    int current;
    int interval;
    void (*handler)(struct timer *);
    void *arg;
};

void timer_interrupt_handler(int ticks);
struct timer *timer_create(int initial, int interval,
                           void (*handler)(struct timer *), void *arg);
void timer_reset(struct timer *timer, int initial, int interval);
void timer_destroy(struct timer *timer);
void timer_init(void);

#endif
