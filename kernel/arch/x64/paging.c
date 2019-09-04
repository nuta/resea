#include <arch.h>
#include <debug.h>
#include <memory.h>
#include <printk.h>
#include <x64/x64.h>

#define NTH_LEVEL_INDEX(level, vaddr) \
    (((vaddr) >> ((((level) -1) * 9) + 12)) & 0x1ff)
#define ENTRY_PADDR(entry) ((entry) &0x7ffffffffffff000)

void page_table_init(struct page_table *pt) {
    void *pml4 = kmalloc(&page_arena);
    inlined_memcpy(pml4, from_paddr(KERNEL_PML4_PADDR), PAGE_SIZE);
    asan_init_area(ASAN_VALID, pml4, PAGE_SIZE);
    pt->pml4 = (paddr_t) into_paddr(pml4);
}

void page_table_destroy(UNUSED struct page_table *pt) {
    // TODO:
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
                void *page = kmalloc(&page_arena);
                if (!page) {
                    PANIC("failed to allocate a page for a page table");
                }

                asan_init_area(ASAN_VALID, page, PAGE_SIZE);
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
            TRACE("link: %p -> %p (flags=0x%x)", vaddr, paddr, attrs);
            table[index] = paddr | attrs;
            asm_invlpg(vaddr + offset);
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

paddr_t resolve_paddr_from_vaddr(
    struct page_table *pt, vaddr_t vaddr) {
    uint64_t *table = from_paddr(pt->pml4);
    int level = 4;
    while (level > 1) {
        int index = NTH_LEVEL_INDEX(level, vaddr);
        if (!table[index]) {
            return 0;
        }

        // Go into the next level paging table.
        table = (uint64_t *) from_paddr(ENTRY_PADDR(table[index]));
        level--;
    }

    // `table` now points to the PT.
    uint64_t entry = table[NTH_LEVEL_INDEX(1, vaddr)];
    if (!entry) {
        return 0;
    }

    return ENTRY_PADDR(entry);
}
