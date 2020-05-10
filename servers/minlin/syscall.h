#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <types.h>

struct proc;
void handle_syscall(struct proc *proc, struct abi_emu_frame *args);
void try_syscall(struct proc *proc);

#endif
