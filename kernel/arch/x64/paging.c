#include <arch.h>
#include <debug.h>
#include <memory.h>
#include <printk.h>
#include <x64/x64.h>

#define NTH_LEVEL_INDEX(level, vaddr) \
    (((vaddr) >> ((((level) -1) * 9) + 12)) & 0x1ff)
#define ENTRY_PADDR(entry) ((entry) &0x7ffffffffffff000)

void page_table_init(struct page_table *pt) {
    uint64_t *pml4 = KMALLOC(&page_arena, PAGE_SIZE);
    inlined_memcpy(pml4, from_paddr(KERNEL_PML4_PADDR), PAGE_SIZE);

    // The kernel no longer access a virtual address around 0x0000_0000. Unmap
    // the area to catch bugs (especially NULL pointer dereferences in the
    // kernel).
    pml4[0] = 0;

#ifdef DEBUG_BUILD
    asan_init_area(ASAN_VALID, pml4, PAGE_SIZE);
#endif
    pt->pml4 = (paddr_t) into_paddr(pml4);
}

void page_table_destroy(UNUSED struct page_table *pt) {
    // TODO: Traverse and kfree the page table.
}

void link_page(struct page_table *pt, vaddr_t vaddr, paddr_t paddr,
               int num_pages, uintmax_t flags) {
    ASSERT(vaddr < KERNEL_BASE_ADDR && "tried to link a kernel page");

    uint64_t attrs = PAGE_PRESENT | flags;
    while (1) {
        uint64_t *table = from_paddr(pt->pml4);
        int level = 4;
        while (level > 1) {
            int index = NTH_LEVEL_INDEX(level, vaddr);
            if (!table[index]) {
                /* The PDPT, PD or PT is not allocated. Allocate it. */
                void *page = KMALLOC(&page_arena, PAGE_SIZE);
                if (!page) {
                    PANIC("failed to allocate a page for a page table");
                }

#ifdef DEBUG_BUILD
                asan_init_area(ASAN_VALID, page, PAGE_SIZE);
#endif
                table[index] = (uint64_t) into_paddr(page);
            }

            // Update attributes.
            table[index] = ENTRY_PADDR(table[index]) | attrs;

            // Go into the next level paging table.
            table = (uint64_t *) from_paddr(ENTRY_PADDR(table[index]));
            level--;
        }

        // `table` now points to the PT.
        int index = NTH_LEVEL_INDEX(1, vaddr);
        int remaining = num_pages;
        uint64_t offset = 0;
        while (remaining > 0 && index < PAGE_ENTRY_NUM) {
            // TRACE("link: %p -> %p (flags=0x%x)", vaddr, paddr, attrs);
            table[index] = paddr | attrs;
            asm_invlpg(vaddr);
            vaddr += PAGE_SIZE;
            paddr += PAGE_SIZE;
            offset += PAGE_SIZE;
            remaining--;
            index++;
        }

        if (!remaining) {
            break;
        }

        // The page spans across multiple PTs.
    }
}

void unlink_page(struct page_table *pt, vaddr_t vaddr, int num_pages) {
    ASSERT(vaddr < KERNEL_BASE_ADDR && "tried to link a kernel page");
    int remaining = num_pages;
    while (remaining > 0) {
        uint64_t *table = from_paddr(pt->pml4);
        int level = 4;
        while (level > 1) {
            int index = NTH_LEVEL_INDEX(level, vaddr);
            ASSERT(table[index]);
            table = (uint64_t *) from_paddr(ENTRY_PADDR(table[index]));
            level--;
        }

        // `table` now points to the PT.
        int index = NTH_LEVEL_INDEX(1, vaddr);
        while (remaining > 0 && index < PAGE_ENTRY_NUM) {
            // TRACE("unlink: %p", vaddr);
            table[index] = 0;
            asm_invlpg(vaddr);
            vaddr += PAGE_SIZE;
            remaining--;
            index++;
        }
    }
}

// TODO: Rewrite this function. It looks ugly :(
paddr_t resolve_paddr_from_vaddr(struct page_table *pt, vaddr_t vaddr) {
    uint64_t *table = from_paddr(pt->pml4);
    int level = 4;
    int i = 4;
    while (i > 1) {
        int index = NTH_LEVEL_INDEX(level, vaddr);
        if (!table[index]) {
            return 0;
        }

        // Go into the next level paging table.
        table = (uint64_t *) from_paddr(ENTRY_PADDR(table[index]));
        i -= (table[index] & 0x80 /* Is bigger page? */) ? 2 : 1;
        level--;
    }

    // `table` now points to the PT (4KiB page) or PD (1MiB page).
    uint64_t entry = table[NTH_LEVEL_INDEX(level, vaddr)];
    if (!entry) {
        return 0;
    }

    uint64_t offset = (level == 2)
        ? NTH_LEVEL_INDEX(level - 1, vaddr) << 12
        : 0;
    return ENTRY_PADDR(entry) + offset;
}
