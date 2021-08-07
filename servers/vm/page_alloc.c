#include "page_alloc.h"
#include "page_fault.h"
#include "task.h"
#include <bootinfo.h>
#include <resea/malloc.h>
#include <resea/printf.h>

extern char __free_vaddr_end[];

size_t num_unused_pages = 0;
static struct page pages[PAGES_MAX];
static list_t regions;

pfn_t paddr2pfn(paddr_t paddr) {
    ASSERT(IS_ALIGNED(paddr, PAGE_SIZE) && paddr >= PAGES_BASE_ADDR);
    return (paddr - PAGES_BASE_ADDR) / PAGE_SIZE;
}

void page_incref(pfn_t pfn, size_t num_pages) {
    ASSERT(pfn + num_pages <= PAGES_MAX);
    for (size_t i = 0; i < num_pages; i++) {
        if (!pages[pfn + i].ref_count) {
            num_unused_pages--;
        }

        pages[pfn + i].ref_count++;
    }
}

void page_decref(pfn_t pfn, size_t num_pages) {
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
paddr_t page_alloc(size_t num_pages) {
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
                page_incref(base, num_pages);
                paddr_t paddr = PAGES_BASE_ADDR + base * PAGE_SIZE;
                return paddr;
            }
        }
    }

    PANIC("out of memory");
}

static bool is_mappable_paddr_range(paddr_t paddr, size_t num_pages) {
    paddr_t paddr_end = paddr + num_pages * PAGE_SIZE;
    return paddr >= PAGES_BASE_ADDR && paddr_end >= PAGES_BASE_ADDR
           && paddr_end < PAGES_BASE_ADDR_END
           && paddr < paddr_end  // No wrapping around.
        ;
}

/// Allocates a memory space for at task. Note that *vaddr and *paddr MUST BE
/// initialized with proper values as described below.
///
/// If `*paddr` is zero, it allocates unused physical memory pages. Otherwise,
/// it maps to the specified physical memory addreess (used by some device
/// drivers to access memory-mapped I/O area).
///
/// If `*vaddr` is zero, it allocates unused virtual memory address in the task.
/// Otherwise, it maps the physical memory pages to the given virtual address.
///
/// `vaddr` can be NULL and if it is, the allocated memory page is marked as
/// non-mappable.
error_t task_page_alloc(struct task *task, vaddr_t *vaddr, paddr_t *paddr,
                        size_t num_pages) {
    if (*paddr) {
        if (!IS_ALIGNED(*paddr, PAGE_SIZE)) {
            WARN_DBG("%s: unaligned paddr %p", __func__, *paddr);
            return ERR_INVALID_ARG;
        }

        if (!is_mappable_paddr_range(*paddr, num_pages)) {
            WARN_DBG("%s: invalid paddr %p", __func__, *paddr);
            return ERR_NOT_ACCEPTABLE;
        }

        // Map the specified physical memory address.
        for (size_t i = 0; i < num_pages; i++) {
            offset_t off = i * PAGE_SIZE;
            error_t err = map_page(task, *vaddr + off, *paddr + off,
                                   MAP_TYPE_READWRITE, false);
            if (err != OK) {
                return err;
            }
        }

        page_incref(paddr2pfn(*paddr), num_pages);
    } else {
        *paddr = page_alloc(num_pages);
    }

    if (vaddr != NULL && !*vaddr) {
        *vaddr = virt_page_alloc(task, num_pages);
        if (!*vaddr) {
            return ERR_NO_MEMORY;
        }
    }

    struct page_area *area = malloc(sizeof(*area));
    area->vaddr = (vaddr != NULL) ? *vaddr : 0;
    area->paddr = *paddr;
    area->num_pages = num_pages;
    list_push_back(&task->page_areas, &area->next);
    return OK;
}

/// Allocates a virtual address space by so-called the bump pointer allocation
/// algorithm. Unlike task_page_alloc(), it doesn't maps to a physical memory
/// pages.
vaddr_t virt_page_alloc(struct task *task, size_t num_pages) {
    vaddr_t vaddr = task->free_vaddr;
    size_t size = num_pages * PAGE_SIZE;

    if (vaddr + size >= (vaddr_t) __free_vaddr_end) {
        // Task's virtual memory space has been exhausted.
        WARN_DBG("%s: run out of virtual memory space", task->name);
        task_kill(task);
        return 0;
    }

    task->free_vaddr += size;
    return vaddr;
}

static void free_page_area(struct page_area *area) {
    page_decref(paddr2pfn(area->paddr), area->num_pages);
    list_remove(&area->next);
    free(area);
}

/// Frees the physical memory pages allocated for the task. `paddr` is the
/// beginning of the allocated physical memory area.
void task_page_free(struct task *task, paddr_t paddr) {
    LIST_FOR_EACH (area, &task->page_areas, struct page_area, next) {
        if (area->paddr == paddr) {
            free_page_area(area);
            return;
        }
    }

    OOPS("failed to free paddr=%p in %s (double free?)", paddr, task->name);
}

/// Frees all memory areas allocated for the task.
void task_page_free_all(struct task *task) {
    LIST_FOR_EACH (area, &task->page_areas, struct page_area, next) {
        free_page_area(area);
    }
}

extern struct bootinfo __bootinfo;

void page_alloc_init(void) {
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
        TRACE("available RAM region #%d: %p-%p (%d%s)", i, region->base,
              region->base + region->num_pages * PAGE_SIZE,
              (size_mb > 0) ? size_mb : size_kb, (size_mb > 0) ? "MiB" : "KiB");

        list_push_back(&regions, &region->next);
    }

    for (pfn_t i = 0; i < PAGES_MAX; i++) {
        pages[i].ref_count = 0;
        num_unused_pages++;
    }
}
