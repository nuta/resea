#ifndef __X64_TRAP_H__
#define __X64_TRAP_H__

#define INTERRUPT_HANDLER_SIZE 16
#define GS_RSP0                0
#define GS_RSP3                8
#define GS_ABI_EMU             16
#define USER_RFLAGS_MASK       0xffffffffffffcfff

#ifndef __ASSEMBLER__
#    include <arch.h>

//
// Handlers defined in trap.S.
//
typedef uint8_t interrupt_handler_t[16];
extern interrupt_handler_t interrupt_handlers[];
void syscall_entry(void);
void userland_entry(void);
void switch_context(uint64_t *prev_rsp, uint64_t *next_rsp);
extern char usercopy[];

STATIC_ASSERT(offsetof(struct arch_cpuvar, rsp0) == GS_RSP0);
STATIC_ASSERT(offsetof(struct arch_cpuvar, rsp3) == GS_RSP3);
STATIC_ASSERT(offsetof(struct arch_cpuvar, abi_emu) == GS_ABI_EMU);
#endif  // ifndef __ASSEMBLER__

#endif
