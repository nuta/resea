#include <memory.h>
#include <syscall.h>
#include <string.h>
#include "vm.h"

extern char __temp_page[];

static uint64_t *traverse_page_table(uint64_t *table, vaddr_t vaddr,
                                     pageattrs_t attrs) {
    ASSERT(vaddr < KERNEL_BASE_ADDR || vaddr == (vaddr_t) __temp_page);
    ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));

    for (int level = 4; level > 1; level--) {
        int index = NTH_LEVEL_INDEX(level, vaddr);
        if (!table[index]) {
            if (!attrs) {
                return NULL;
            }

            void *page = kmalloc(PAGE_SIZE);
            if (!page) {
                return NULL;
            }

            memset(page, 0, PAGE_SIZE);
            table[index] = (uint64_t) into_paddr(page);
        }

        // Update attributes if given.
        table[index] |= attrs | ARM64_PAGE_ACCESS | ARM64_PAGE_TABLE;

        // Go into the next level paging table.
        table = (uint64_t *) from_paddr(ENTRY_PADDR(table[index]));
    }

    return &table[NTH_LEVEL_INDEX(1, vaddr)];
}

error_t vm_create(struct vm *vm) {
    vm->entries = kmalloc(PAGE_SIZE);
    if (!vm->entries) {
        return ERR_NO_MEMORY;
    }

    memset(vm->entries, 0, PAGE_SIZE);
    vm->ttbr0 = into_paddr(vm->entries);
    return OK;
}

void vm_destroy(struct vm *vm) {
    // TODO:
}

error_t vm_link(struct vm *vm, vaddr_t vaddr, paddr_t paddr,
                pageattrs_t attrs) {
    ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));

    attrs |= PAGE_PRESENT;
    uint64_t *entry = traverse_page_table(vm->entries, vaddr, attrs);
    if (!entry) {
        return ERR_NO_MEMORY;
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
    // TODO:
}

paddr_t vm_resolve(struct vm *vm, vaddr_t vaddr) {
    uint64_t *entry = traverse_page_table(vm->entries, vaddr, 0);
    return (entry) ? ENTRY_PADDR(*entry) : 0;
}
