#pragma once
#include <list.h>
#include <types.h>

// Flags for pm_alloc().
#define PM_ALLOC_UNINITIALIZED (1 << 0)
#define PM_ALLOC_ZEROED        (0 << 0)
#define PM_ALLOC_ALIGNED       (1 << 1)

#define PAGE_TYPE_KERNEL     1
#define PAGE_TYPE_USER(task) ((task)->tid << 1)

struct page {
    unsigned type;
    unsigned ref_count;
};

struct memory_zone {
    list_elem_t next;
    paddr_t base;
    size_t num_pages;
    struct page pages[0];
};

struct task;

paddr_t pm_alloc(size_t size, unsigned type, unsigned flags);
void pm_free(paddr_t paddr, size_t size);
error_t vm_map(struct task *task, uaddr_t uaddr, paddr_t paddr, size_t size,
               unsigned attrs);
void handle_page_fault(uaddr_t uaddr, vaddr_t ip, unsigned fault);

struct bootinfo;
void memory_init(struct bootinfo *bootinfo);
