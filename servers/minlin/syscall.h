#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <types.h>

struct proc;
void handle_syscall(struct proc *proc, trap_frame_t *args);
void try_syscall(struct proc *proc);

#endif
