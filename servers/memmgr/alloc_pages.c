#include <init_args.h>
#include <list.h>
#include <resea.h>
#include <resea/math.h>
#include "alloc_pages.h"
#include "memory_map.h"

static struct zone zones[MAX_NUM_ZONES];
static int num_zones;

static size_t compute_buddy_pfn(size_t pfn, int order) {
    return pfn ^ (1ULL << order);
}

static size_t compute_merged_pfn(size_t pfn, size_t buddy_pfn) {
    return pfn & buddy_pfn;
}

static struct page *pfn_to_page(struct zone *zone, size_t pfn) {
    assert(pfn < zone->num_pages);
    return &zone->pages[pfn];
}

static size_t paddr_to_pfn(struct zone *zone, paddr_t paddr) {
    assert((paddr & (PAGE_SIZE - 1)) == 0);
    return (paddr - zone->base_addr) >> (ulllog2(PAGE_SIZE) - 1 /* FIXME: */);
}

static bool is_paddr_in_zone(struct zone *zone, paddr_t paddr) {
    return paddr >= zone->base_addr
        && paddr_to_pfn(zone, paddr) < zones->num_pages;
}

struct page *paddr_to_page(struct zone *zone, paddr_t paddr) {
    assert(is_paddr_in_zone(zone, paddr));
    return &zones->pages[paddr_to_pfn(zone, paddr)];
}

static paddr_t page_to_paddr(struct zone *zone, struct page *page) {
    assert((uintptr_t) page >= (uintptr_t) zone->pages);
    size_t index = ((uintptr_t) page - (uintptr_t) zone->pages) / sizeof(*page);
    assert(index < zone->num_pages);
    return zone->base_addr + index * PAGE_SIZE;
}

static size_t page_to_pfn(struct zone *zone, struct page *page) {
    return paddr_to_pfn(zone, page_to_paddr(zone, page));
}

static struct page *alloc_from_zone(struct zone *zone, int order) {
    TRACE("alloc_from_zone: Allocating zone=%d, order=%d", zone->id, order);
    struct list_head *free_list = &zone->free_lists[order];
    if (list_is_empty(free_list)) {
        return NULL;
    }

    struct page *page = LIST_CONTAINER(page, next, list_pop_front(free_list));
    if (!page) {
        return NULL;
    }

    assert(page->order == order);
    assert(!page->in_use);

    page->in_use = true;
    TRACE("alloc_from_zone: Allocated zone=%d, pfn=%x, order=%d",
          zone->id, page_to_pfn(zone, page), order);
    return page;
}

static void add_to_free_list(struct zone *zone, struct page *page, int order) {
    TRACE("add_to_free_list: zone=%d, pfn=%x, order=%d",
          zone->id, page_to_pfn(zone, page), order);
    assert(order >= 0);
    page->in_use = false;
    page->order = order;
    list_push_back(&zone->free_lists[order], &page->next);
}

static error_t split(struct zone *zone, int target_order) {
    TRACE("split: target_order=%d", target_order);

    if (!list_is_empty(&zone->free_lists[target_order])) {
        return OK;
    }

    assert(target_order < MAX_PAGE_ORDER);

    int from;
    for (from = target_order + 1; from <= MAX_PAGE_ORDER; from++) {
        if (!list_is_empty(&zone->free_lists[from])) {
            break;
        }
    }

    if (from > MAX_PAGE_ORDER) {
        return ERR_OUT_OF_MEMORY;
    }

    for (; from > target_order; from--) {
        // Allocate a chunk from the higher order (`from`) and use it as two
        // chunks in the current (`crnt`) order.
        int crnt = from - 1;
        struct page *page = alloc_from_zone(zone, from);
        assert(page);
        struct page *buddy_page = pfn_to_page(zone, compute_buddy_pfn(
                                      page_to_pfn(zone, page), crnt));
        TRACE("split: split zone=%d, pfn=%x order=%d into two order=%d chunks",
              zone->id, page_to_pfn(zone, page), from, crnt);
        add_to_free_list(zone, page, crnt);
        add_to_free_list(zone, buddy_page, crnt);
    }

    return OK;
}

static void merge(struct zone *zone, size_t pfn, int order) {
    assert(!pfn_to_page(zone, pfn)->in_use);
    return;
    while (order < MAX_PAGE_ORDER) {
        size_t buddy_pfn = compute_buddy_pfn(pfn, order);
        // Check whether the computed PFN is valid.
        if (buddy_pfn >= zone->num_pages) {
            break;
        }

        // Check whether the buddy is ready to be merged.
        struct page *buddy = pfn_to_page(zone, buddy_pfn);
        if (buddy->in_use || buddy->order != order) {
            break;
        }

        // Merge with the buddy.
        size_t merged_pfn = compute_merged_pfn(pfn, buddy_pfn);
        struct page *merged_page = pfn_to_page(zone, merged_pfn);
        list_remove(&merged_page->next);
        add_to_free_list(zone, merged_page, order);
        pfn = merged_pfn;
        order++;
    }
}

/// Allocates 2^order pages.
uintptr_t alloc_pages(int order) {
    for (int i = 0; i < num_zones; i++) {
        struct zone *zone = &zones[i];
        struct page *page;
retry:
        page = alloc_from_zone(zone, order);
        if (page) {
            TRACE("alloc_pages: Allocated paddr=%p", page_to_paddr(zone, page));
            return page_to_paddr(zone, page);
        }

        // Failed to allocate. Try filling the free list for the target order by
        // splitting larger order.
        if (split(zone, order) == OK) {
            goto retry;
        }
    }

    WARN("alloc_pages: run out of memory");
    return 0;
}

void free_pages(paddr_t paddr, int order) {
    struct zone *zone = NULL;
    for (int i = 0; i < num_zones; i++) {
        if (is_paddr_in_zone(&zones[i], paddr)) {
            zone = &zones[i];
            break;
        }
    }

    assert(zone);

    // Free the given pages.
    size_t pfn = paddr_to_pfn(zone, paddr);
    add_to_free_list(zone, pfn_to_page(zone, pfn), order);

    merge(zone, pfn, order);
}

static void init_page(struct page *page) {
    page->in_use = false;
}

static void init_zone(struct zone *zone, int id, paddr_t start, paddr_t end) {
    size_t num_total_pages = (end - start) / PAGE_SIZE;
    size_t num_internal_pages = (num_total_pages / 100) * INTERNAL_PAGES_PERC;
    size_t num_data_pages = num_total_pages - num_internal_pages;
    TRACE("init_zone: %d internal pages (%dMiB), %d data pages (%dMiB)",
          num_internal_pages,
          (num_internal_pages * PAGE_SIZE) / 1024 / 1024,
          num_data_pages,
          (num_data_pages * PAGE_SIZE) / 1024 / 1024);

    assert(num_internal_pages > 0);
    assert((num_internal_pages * PAGE_SIZE) / sizeof(struct page)
        >= num_data_pages);

    struct page *pages = (struct page *) start;
    zone->id = id;
    zone->pages = pages;
    zone->num_pages = num_data_pages;
    zone->base_addr = start + num_internal_pages * PAGE_SIZE;
    for (int i = 0; i <= MAX_PAGE_ORDER; i++) {
        list_init(&zone->free_lists[i]);
    }

    for (size_t i = 0; i < zone->num_pages; i++) {
        init_page(&pages[i]);
    }

    intmax_t remaining = zone->num_pages;
    for (int order = MAX_PAGE_ORDER; remaining > 0 && order >= 0; order--) {
        size_t pages_per_entry = POW2(order);
        size_t num_entries = remaining / pages_per_entry;
        size_t base = num_data_pages - remaining;
        for (size_t i = 0; i < num_entries; i++) {
            add_to_free_list(zone, &pages[base + i * pages_per_entry], order);
        }
        remaining -= num_entries * pages_per_entry;
    }
}

void init_alloc_pages(struct memory_map *memory_maps, int num_memory_maps) {
    INFO("Memory Map:");
    int j = 0;
    for (int i = 0; i < num_memory_maps; i++) {
        struct memory_map *mmap = &memory_maps[i];
        INFO("    %p-%p: (%s, %dMiB)", mmap->start, mmap->end,
            (mmap->type == INITARGS_MEMORY_MAP_FREE) ? "free" : "reserved",
            (mmap->end - mmap->start) / 1024 / 1024);

        if (mmap->type == INITARGS_MEMORY_MAP_FREE) {
            assert(j < MAX_NUM_ZONES);
            init_zone(&zones[j], j, MAX(mmap->start, FREE_MEMORY_START),
                      mmap->end);
            j++;
        }
    }

    num_zones = j;
}
