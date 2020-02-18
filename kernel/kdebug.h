#ifndef __KDEBUG_H__
#define __KDEBUG_H__

void kdebug_handle_interrupt(void);
void stack_check(void);
void stack_set_canary(void);

// Implemented in arch.
int kdebug_readchar(void);

#endif
