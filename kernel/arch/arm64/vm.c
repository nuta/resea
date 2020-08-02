#include <arch.h>
#include <syscall.h>
#include <printk.h>
#include <string.h>
#include "vm.h"

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

            if (!kpage) {
                return NULL;
            }

            memset(from_paddr(kpage), 0, PAGE_SIZE);
            table[index] = kpage;
            kpage = 0;
        }

        // Update attributes if given.
        table[index] |= attrs | ARM64_PAGE_ACCESS | ARM64_PAGE_TABLE;

        // Go into the next level paging table.
        table = (uint64_t *) from_paddr(ENTRY_PADDR(table[index]));
    }

    return &table[NTH_LEVEL_INDEX(1, vaddr)];
}

error_t vm_create(struct vm *vm) {
    memset(vm->entries, 0, PAGE_SIZE);
    vm->ttbr0 = into_paddr(vm->entries);
    return OK;
}

void vm_destroy(struct vm *vm) {
    // TODO:
}

error_t vm_link(struct vm *vm, vaddr_t vaddr, paddr_t paddr, paddr_t kpage,
                unsigned flags) {
    ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));

    uint64_t attrs = 1 << 6; // user
    // TODO: MAP_W

    uint64_t *entry = traverse_page_table(vm->entries, vaddr, kpage, attrs);
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

void vm_unlink(struct vm *vm, vaddr_t vaddr) {
    uint64_t *entry = traverse_page_table(vm->entries, vaddr, 0, 0);
    if (entry) {
        *entry = 0;
        // FIXME: Flush only the affected page.
        __asm__ __volatile__("dsb ish");
        __asm__ __volatile__("isb");
        __asm__ __volatile__("tlbi vmalle1is");
        __asm__ __volatile__("dsb ish");
        __asm__ __volatile__("isb");
    }
}

paddr_t vm_resolve(struct vm *vm, vaddr_t vaddr) {
    uint64_t *entry = traverse_page_table(vm->entries, vaddr, 0, 0);
    return (entry) ? ENTRY_PADDR(*entry) : 0;
}
