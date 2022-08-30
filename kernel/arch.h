#pragma once
#include <types.h>

struct cpuvar;
#include <arch_types.h>

struct task;

struct cpuvar {
    struct arch_cpuvar arch;
    struct task *idle_task;
    struct task *current_task;
};

enum memory_map_type {
    MEMORY_MAP_FREE,
};

struct memory_map_entry {
    enum memory_map_type type;
    paddr_t paddr;
    size_t size;
};

#define NUM_MEMORY_MAP_ENTRIES_MAX 8

struct memory_map {
    struct memory_map_entry entries[NUM_MEMORY_MAP_ENTRIES_MAX];
    int num_entries;
};

struct bootinfo {
    struct memory_map memory_map;
};

void arch_console_write(char ch);
error_t arch_vm_init(struct arch_vm *vm);
error_t arch_vm_map(struct arch_vm *vm, vaddr_t vaddr, paddr_t paddr,
                    size_t size, unsigned attrs);
void *arch_paddr2ptr(paddr_t paddr);
error_t arch_task_init(struct task *task, uaddr_t ip);
void arch_task_switch(struct task *prev, struct task *next);
