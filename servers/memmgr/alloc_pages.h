#ifndef __ALLOC_PAGES_H__
#define __ALLOC_PAGES_H__

#include <list.h>
#include <resea.h>

#define MAX_NUM_ZONES 16
#define MAX_PAGE_ORDER 12
#define FREE_LIST_TO_PAGE(free_list) ((struct page *) (free_list))

struct page {
    bool in_use;
    int order;
    struct list_head next;
};

#define INTERNAL_PAGES_PERC 1
STATIC_ASSERT(sizeof(struct page) <= 40);

struct zone {
    struct list_head free_lists[MAX_PAGE_ORDER + 1];
    int id;
    struct page *pages;
    size_t num_pages;
    paddr_t base_addr;
    paddr_t start;
    paddr_t end;
};

paddr_t alloc_pages(int order);
void free_pages(paddr_t paddr, int order);
struct memory_map;
void init_alloc_pages(struct memory_map *memory_maps, int num_memory_maps);

#endif
