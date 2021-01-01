#ifndef __ALLOC_PAGE_H__
#define __ALLOC_PAGE_H__

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
void pages_incref(pfn_t pfn, size_t num_pages);
void pages_decref(pfn_t pfn, size_t num_pages);
paddr_t pages_alloc(size_t num_pages);
void pages_init(void);

#endif
