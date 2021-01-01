#ifndef __MAIN_H__
#define __MAIN_H__

#include <types.h>

struct bootinfo;
__noreturn void kmain(struct bootinfo *bootinfo);
__noreturn void mpmain(void);

// Implemented in arch.
void mp_start(void);
__noreturn void arch_idle(void);
void halt(void);

#endif
