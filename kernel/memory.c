#include "memory.h"

static list_t zones;

static bool is_contiguously_free(struct memory_zone *zone, size_t start, size_t num_pages) {
    for (size_t i = start; i < zone->num_pages; i++) {
        if (zone->pages[i].ref_count != 0) {
            return false;
        }
    }
    return true;
}

paddr_t pm_alloc(struct task *task, size_t num_pages) {
    LIST_FOR_EACH (zone, &zones, struct memory_zone, next) {
        for (size_t start = 0; start < zone->num_pages; start++) {
            if (is_contiguously_free(zone, start, num_pages)) {
                for (size_t i = 0; i < num_pages; i++) {
                    zone->pages[start + i].ref_count = 1;
                    zone->pages[start + i].owner = task;
                }

                return zone->base + start * PAGE_SIZE;
            }
        }
    }

    return 0;
}

void pm_add_zone(paddr_t paddr, size_t len) {
    // FIXME: paddr to vaddr
    struct memory_zone *zone = (struct memory_zone *) paddr;
    // TODO: Is this correct?
    size_t num_pages = ALIGN_UP(len, PAGE_SIZE) / PAGE_SIZE;

    void *end_of_header = &zone->pages[num_pages + 1];
    size_t header_size = ((vaddr_t) end_of_header) - ((vaddr_t) zone);

    zone->base = paddr + ALIGN_UP(header_size, PAGE_SIZE);
    zone->num_pages = num_pages;
    for (size_t i = 0; i < num_pages; i++) {
        zone->pages[i].ref_count = 0;
    }

    list_push_back(&zones, &zone->next);
}
