#ifndef __TIMER_H__
#define __TIMER_H__

#define TICK_HZ 1000
#define THREAD_SWITCH_INTERVAL ((20 * 1000) / TICK_HZ)
void timer_interrupt_handler(void);

#endif
