#include "memory.h"
#include "arch.h"
#include "printk.h"
#include <string.h>

static list_t zones = LIST_INIT(zones);

static bool is_contiguously_free(struct memory_zone *zone, size_t start,
                                 size_t num_pages) {
    for (size_t i = start; i < zone->num_pages; i++) {
        if (zone->pages[i].ref_count != 0) {
            return false;
        }
    }
    return true;
}

static struct page *find_page_by_paddr(paddr_t paddr) {
    LIST_FOR_EACH(zone, &zones, struct memory_zone, next) {
        if (zone->base <= paddr
            && paddr < zone->base + zone->num_pages * PAGE_SIZE) {
            size_t start = (paddr - zone->base) / PAGE_SIZE;
            return &zone->pages[start];
        }
    }

    return NULL;
}

static void add_zone(paddr_t paddr, size_t size) {
    // FIXME: paddr to vaddr
    struct memory_zone *zone = (struct memory_zone *) paddr;
    // TODO: Is this correct?
    size_t num_pages = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;

    void *end_of_header = &zone->pages[num_pages + 1];
    size_t header_size = ((vaddr_t) end_of_header) - ((vaddr_t) zone);

    zone->base = paddr + ALIGN_UP(header_size, PAGE_SIZE);
    zone->num_pages = num_pages;
    for (size_t i = 0; i < num_pages; i++) {
        zone->pages[i].ref_count = 0;
    }

    list_push_back(&zones, &zone->next);
}

paddr_t pm_alloc(size_t size, unsigned type, unsigned flags) {
    size_t num_pages = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
    LIST_FOR_EACH(zone, &zones, struct memory_zone, next) {
        for (size_t start = 0; start < zone->num_pages; start++) {
            if (is_contiguously_free(zone, start, num_pages)) {
                for (size_t i = 0; i < num_pages; i++) {
                    zone->pages[start + i].ref_count = 1;
                    zone->pages[start + i].type = type;
                }

                paddr_t paddr = zone->base + start * PAGE_SIZE;
                if (flags & PM_ALLOC_UNINITIALIZED) {
                    memset(arch_paddr2ptr(paddr), 0, PAGE_SIZE * num_pages);
                }

                return paddr;
            }
        }
    }

    return 0;
}

void pm_free(paddr_t paddr, size_t num_pages) {
    struct page *page = find_page_by_paddr(paddr);
    ASSERT(page != NULL);

    for (size_t i = 0; i < num_pages; i++) {
        DEBUG_ASSERT(page->ref_count > 0);
        page->ref_count--;
    }
}

void memory_init(struct bootinfo *bootinfo) {
    struct memory_map *memory_map = &bootinfo->memory_map;
    for (int i = 0; i < memory_map->num_entries; i++) {
        add_zone(memory_map->entries[i].paddr, memory_map->entries[i].size);
    }
}
