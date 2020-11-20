#include <resea/printf.h>
#include "pages.h"

size_t num_unused_pages = 0;
static struct page pages[PAGES_MAX];

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
    pfn_t i;
    for (i = 0; i < PAGES_MAX; i++) {
        size_t j = 0;
        while (j < num_pages) {
            if (pages[i + j].ref_count > 0) {
                break;
            }

            j++;
        }

        if (j == num_pages) {
            // Found sufficiently large free physical pages.
            pages_incref(i, num_pages);
            paddr_t paddr = PAGES_BASE_ADDR + i * PAGE_SIZE;
            return paddr;
        }
    }

    PANIC("out of memory");
}

void pages_init(void) {
    for (pfn_t i = 0; i < PAGES_MAX; i++) {
        pages[i].ref_count = 0;
        num_unused_pages++;
    }
}
