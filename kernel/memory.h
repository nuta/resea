#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <list.h>
#include <types.h>

struct page {
    unsigned ref_count;
    struct task *owner;
};

struct memory_zone {
    list_elem_t next;
    paddr_t base;
    size_t num_pages;
    struct page pages[0];
};

#endif
