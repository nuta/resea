#pragma once
#include <types.h>
#include <arch_types.h>

void arch_console_write(const char *str);
void arch_vm_init(struct arch_vm *vm);
void arch_vm_map(struct arch_vm *vm, uaddr_t uaddr, paddr_t paddr, unsigned flags);
void *arch_paddr2ptr(paddr_t paddr);
