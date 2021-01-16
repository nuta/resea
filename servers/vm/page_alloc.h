#ifndef __PAGE_ALLOC_H__
#define __PAGE_ALLOC_H__

#include <list.h>
#include <types.h>

/// Page Frame Number.
typedef unsigned pfn_t;
#define PAGES_MAX ((4ULL * 1024 * 1024 * 1024) / PAGE_SIZE)

struct page {
    unsigned ref_count;
};

struct available_ram_region {
    list_elem_t next;
    paddr_t base;
    size_t num_pages;
};

extern char __straight_mapping[];
#define PAGES_BASE_ADDR     ((paddr_t) __straight_mapping)
#define PAGES_BASE_ADDR_END (PAGES_MAX * PAGE_SIZE)

extern size_t num_unused_pages;

pfn_t paddr2pfn(paddr_t paddr);
void page_incref(pfn_t pfn, size_t num_pages);
void page_decref(pfn_t pfn, size_t num_pages);
paddr_t page_alloc(size_t num_pages);
struct task;
error_t task_page_alloc(struct task *task, vaddr_t *vaddr, paddr_t *paddr,
                        size_t num_pages);
vaddr_t virt_page_alloc(struct task *task, size_t num_pages);
void task_page_free(struct task *task, paddr_t paddr);
void task_page_free_all(struct task *task);
void page_alloc_init(void);

#endif
