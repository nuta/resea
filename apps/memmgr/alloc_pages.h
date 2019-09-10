#ifndef __ALLOC_PAGES_H__
#define __ALLOC_PAGES_H__

#include <list.h>
#include <resea.h>

#define MAX_NUM_ZONES 16
#define MAX_PAGE_ORDER 12

struct zone {
    uintptr_t next_alloc_addr;
    size_t remaining_bytes;
};

uintptr_t do_alloc_pages(int order);
void init_alloc_pages(void);

#endif
