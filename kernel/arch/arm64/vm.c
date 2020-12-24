#include "vm.h"
#include <arch.h>
#include <printk.h>
#include <string.h>
#include <syscall.h>
#include <task.h>

static uint64_t *traverse_page_table(uint64_t *table, vaddr_t vaddr,
                                     paddr_t kpage, uint64_t attrs) {
    ASSERT(vaddr < KERNEL_BASE_ADDR);
    ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));
    ASSERT(IS_ALIGNED(kpage, PAGE_SIZE));

    for (int level = 4; level > 1; level--) {
        int index = NTH_LEVEL_INDEX(level, vaddr);
        if (!table[index]) {
            if (!attrs) {
                return NULL;
            }

            memset(from_paddr(kpage), 0, PAGE_SIZE);
            table[index] = kpage;
            return NULL;
        }

        // Update attributes if given.
        table[index] |= attrs | ARM64_PAGE_ACCESS | ARM64_PAGE_TABLE;

        // Go into the next level paging table.
        table = (uint64_t *) from_paddr(ENTRY_PADDR(table[index]));
    }

    return &table[NTH_LEVEL_INDEX(1, vaddr)];
}

error_t arch_vm_map(struct task *task, vaddr_t vaddr, paddr_t paddr,
                    paddr_t kpage, unsigned flags) {
    ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));

    uint64_t attrs;
    switch (MAP_TYPE(flags)) {
        case MAP_TYPE_READONLY:
            attrs = ARM64_PAGE_MEMATTR_READONLY;
            break;
        case MAP_TYPE_READWRITE:
            attrs = ARM64_PAGE_MEMATTR_READWRITE;
            break;
        default:
            UNREACHABLE();
    }

    uint64_t *entry =
        traverse_page_table(task->arch.page_table, vaddr, kpage, attrs);
    if (!entry) {
        return (kpage) ? ERR_TRY_AGAIN : ERR_EMPTY;
    }

    *entry = paddr | attrs | ARM64_PAGE_ACCESS | ARM64_PAGE_TABLE;

    // FIXME: Flush only the affected page.
    __asm__ __volatile__("dsb ish");
    __asm__ __volatile__("isb");
    __asm__ __volatile__("tlbi vmalle1is");
    __asm__ __volatile__("dsb ish");
    __asm__ __volatile__("isb");
    return OK;
}

error_t arch_vm_unmap(struct task *task, vaddr_t vaddr) {
    uint64_t *entry = traverse_page_table(task->arch.page_table, vaddr, 0, 0);
    if (!entry) {
        return ERR_NOT_FOUND;
    }

    *entry = 0;
    // FIXME: Flush only the affected page.
    __asm__ __volatile__("dsb ish");
    __asm__ __volatile__("isb");
    __asm__ __volatile__("tlbi vmalle1is");
    __asm__ __volatile__("dsb ish");
    __asm__ __volatile__("isb");
    return OK;
}

paddr_t vm_resolve(struct task *task, vaddr_t vaddr) {
    uint64_t *entry = traverse_page_table(task->arch.page_table, vaddr, 0, 0);
    return (entry) ? ENTRY_PADDR(*entry) : 0;
}
