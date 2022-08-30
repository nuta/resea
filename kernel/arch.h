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

struct memory_map {
    enum memory_map_type type;
    paddr_t paddr;
    size_t size;
};

#define NUM_MEMORY_MAPS_MAX 8

struct bootinfo {
    struct memory_map memory_maps[NUM_MEMORY_MAPS_MAX];
    int num_memory_maps;
};

void arch_console_write(char ch);
void arch_vm_init(struct arch_vm *vm);
error_t arch_vm_map(struct arch_vm *vm, vaddr_t vaddr, paddr_t paddr,
                    unsigned attrs);
void *arch_paddr2ptr(paddr_t paddr);
void arch_task_switch(struct task *prev, struct task *next);
