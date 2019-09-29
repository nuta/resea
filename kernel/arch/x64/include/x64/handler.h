#ifndef __X64_HANDLER_H__
#define __X64_HANDLER_H__

#define INTERRUPT_HANDLER_SIZE 16

#ifndef __ASSEMBLER__

#include <types.h>

struct interrupt_handler {
    uint8_t code[INTERRUPT_HANDLER_SIZE];
} PACKED;
STATIC_ASSERT(sizeof(struct interrupt_handler) == INTERRUPT_HANDLER_SIZE);

extern struct interrupt_handler interrupt_handlers[];

void x64_syscall_handler(void);
void x64_switch(struct arch_thread *prev, struct arch_thread *next);
void x64_start_user_thread(void);
void x64_start_kernel_thread(void);

#endif // __ASSEMBLER__

#endif
