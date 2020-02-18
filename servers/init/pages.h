#ifndef __ALLOC_PAGE_H__
#define __ALLOC_PAGE_H__

#include <types.h>

/// Page Frame Number.
typedef unsigned pfn_t;
#define PAGES_MAX ((4ULL * 1024 * 1024 * 1024) / PAGE_SIZE)

struct page {
    unsigned ref_count;
};

bool is_mappable_paddr(paddr_t paddr);
pfn_t paddr2pfn(paddr_t paddr);
void pages_incref(pfn_t pfn, size_t num_pages);
paddr_t pages_alloc(size_t num_pages);
void pages_init(void);

#endif
