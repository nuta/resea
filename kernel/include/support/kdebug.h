#ifndef __KDEBUG_H__
#define __KDEBUG_H__

void debugger_oninterrupt(void);

#ifdef DEBUG_BUILD
#define SET_KDEBUG_INFO(container, field, value) \
    (container)->debug.field = value
#else
#define SET_KDEBUG_INFO(thread, field, value)
#endif

#endif
