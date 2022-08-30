#pragma once
#include <types.h>
#include <arch_types.h>

struct task;

void arch_console_write(char ch);
void arch_vm_init(struct arch_vm *vm);
error_t arch_vm_map(struct arch_vm *vm, vaddr_t vaddr, paddr_t paddr, unsigned attrs);
void *arch_paddr2ptr(paddr_t paddr);
void arch_task_switch(struct task *prev, struct task *next);
