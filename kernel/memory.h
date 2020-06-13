#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <arch.h>
#include <list.h>
#include <types.h>

/// Unused element.
struct free_list {
    uint64_t magic1;
    list_elem_t next;
    size_t num_pages;
    uint64_t magic2;
};

#define FREE_LIST_MAGIC1   0xdeaddead
#define FREE_LIST_MAGIC2   0xbadbadba
#define STACK_CANARY_VALUE 0xdeadca71

void *kmalloc(size_t size);
void kfree(void *ptr);
paddr_t handle_page_fault(vaddr_t addr, vaddr_t ip, pagefault_t fault);
void memory_init(void);

// Implemented in arch.
struct vm;
error_t vm_create(struct vm *vm);
void vm_destroy(struct vm *vm);
error_t vm_link(struct vm *vm, vaddr_t vaddr, paddr_t paddr, pageattrs_t attrs);
paddr_t vm_resolve(struct vm *vm, vaddr_t vaddr);

#endif
