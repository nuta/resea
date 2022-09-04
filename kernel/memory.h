#pragma once
#include <list.h>
#include <types.h>

// Flags for pm_alloc().
#define PM_ALLOC_UNINITIALIZED (1 << 0)
#define PM_ALLOC_ZEROED        (0 << 0)

#define PAGE_TYPE_KERNEL     1
#define PAGE_TYPE_USER(task) ((task)->tid << 1)

#define PAGE_READABLE   (1 << 1)
#define PAGE_WRITABLE   (1 << 2)
#define PAGE_EXECUTABLE (1 << 3)
#define PAGE_USER       (1 << 4)

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
error_t vm_map(struct task *task, uaddr_t uaddr, paddr_t paddr, size_t size,
               unsigned attrs);
void handle_page_fault(uaddr_t uaddr, vaddr_t ip, unsigned fault);

struct bootinfo;
void memory_init(struct bootinfo *bootinfo);
