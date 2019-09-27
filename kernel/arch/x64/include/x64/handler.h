#ifndef __X64_HANDLER_H__
#define __X64_HANDLER_H__

extern char interrupt_handlers[];
void x64_syscall_handler(void);
void x64_switch(struct arch_thread *prev, struct arch_thread *next);
void x64_enter_userspace(void);
void x64_start_kernel_thread(void);

#endif
