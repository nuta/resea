#include <resea/printf.h>
#include <resea/malloc.h>
#include <bootinfo.h>
#include "pages.h"

size_t num_unused_pages = 0;
static struct page pages[PAGES_MAX];
static list_t regions;

pfn_t paddr2pfn(paddr_t paddr) {
    ASSERT(IS_ALIGNED(paddr, PAGE_SIZE) && paddr >= PAGES_BASE_ADDR);
    return (paddr - PAGES_BASE_ADDR) / PAGE_SIZE;
}

void pages_incref(pfn_t pfn, size_t num_pages) {
    ASSERT(pfn + num_pages <= PAGES_MAX);
    for (size_t i = 0; i < num_pages; i++) {
        if (!pages[pfn + i].ref_count) {
            num_unused_pages--;
        }

        pages[pfn + i].ref_count++;
    }
}

void pages_decref(pfn_t pfn, size_t num_pages) {
    ASSERT(pfn + num_pages <= PAGES_MAX);
    for (size_t i = 0; i < num_pages; i++) {
        ASSERT(pages[pfn + i].ref_count > 0);
        pages[pfn + i].ref_count--;

        if (!pages[pfn + i].ref_count) {
            num_unused_pages++;
        }
    }
}

/// Allocates continuous physical memory pages. It always returns a valid
/// physical address: when it runs out of memory, it panics.
paddr_t pages_alloc(size_t num_pages) {
    LIST_FOR_EACH (region, &regions, struct available_ram_region, next) {
        pfn_t first = paddr2pfn(region->base);
        pfn_t end = first + region->num_pages;
        for (pfn_t base = first; base < end; base++) {
            size_t i = 0;
            while (base + i < end) {
                if (pages[base + i].ref_count > 0) {
                    break;
                }

                i++;
                if (i == num_pages) {
                    break;
                }
            }

            if (i == num_pages) {
                // Found sufficiently large free physical pages.
                pages_incref(base, num_pages);
                paddr_t paddr = PAGES_BASE_ADDR + base * PAGE_SIZE;
                return paddr;
            }
        }
    }

    PANIC("out of memory");
}

extern struct bootinfo __bootinfo;

void pages_init(void) {
    struct bootinfo_memmap_entry *m =
        (struct bootinfo_memmap_entry *) &__bootinfo.memmap;

    list_init(&regions);
    for (int i = 0; i < NUM_BOOTINFO_MEMMAP_MAX; i++, m++) {
        if (m->type != BOOTINFO_MEMMAP_TYPE_AVAILABLE) {
            continue;
        }

        if (m->base + m->len < PAGES_BASE_ADDR) {
            continue;
        }

        struct available_ram_region *region = malloc(sizeof(*region));
        if (m->base < PAGES_BASE_ADDR) {
            region->base = PAGES_BASE_ADDR;
            size_t len = m->len - (PAGES_BASE_ADDR - m->base);
            region->num_pages = len / PAGE_SIZE;
        } else {
            region->base = m->base;
            region->num_pages = m->len / PAGE_SIZE;
        }

        size_t size_kb = (region->num_pages * PAGE_SIZE) / 1024;
        size_t size_mb = size_kb / 1024;
        TRACE("available RAM region #%d: %p-%p (%d%s)",
              i,
              region->base,
              region->base + region->num_pages * PAGE_SIZE,
              (size_mb > 0) ? size_mb : size_kb,
              (size_mb > 0) ? "MiB" : "KiB");

        list_push_back(&regions, &region->next);
    }

    for (pfn_t i = 0; i < PAGES_MAX; i++) {
        pages[i].ref_count = 0;
        num_unused_pages++;
    }
}
