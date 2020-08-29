#ifndef __KDEBUG_H__
#define __KDEBUG_H__

#include <types.h>

#define STACK_CANARY_VALUE 0xdeadca71

void kdebug_handle_interrupt(void);
__mustuse error_t kdebug_run(const char *cmdline, char *buf, size_t len);
struct task;
void kdebug_task_destroy(struct task *task);
void stack_check(void);
void stack_set_canary(void);

// Implemented in arch.
int kdebug_readchar(void);
bool kdebug_is_readable(void);
void arch_semihosting_halt(void);

#endif
