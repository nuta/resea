#ifndef __MAIN_H__
#define __MAIN_H__

void kmain(void);
void mpmain(void);

// Implemented in arch.
void mp_start(void);
void arch_idle(void);
void halt(void);

#endif
