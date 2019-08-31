#ifndef __X64_SWITCH_H__
#define __X64_SWITCH_H__

#include <types.h>

void x64_switch(struct arch_thread *prev, struct arch_thread *next);
void x64_enter_userspace(void);
void x64_start_kernel_thread(void);

#endif
