#pragma once
#include <types.h>
#include <arch_types.h>

void arch_console_write(const char *str);
void arch_vm_init(struct arch_vm *vm);
error_t arch_vm_map(struct arch_vm *vm, uaddr_t uaddr, paddr_t paddr, unsigned attrs);
void *arch_paddr2ptr(paddr_t paddr);
