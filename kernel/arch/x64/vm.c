#include <arch.h>
#include <memory.h>
#include <printk.h>
#include <string.h>
#include "vm.h"

static uint64_t *traverse_page_table(uint64_t pml4, vaddr_t vaddr,
                                     pageattrs_t attrs) {
    uint64_t *table = from_paddr(pml4);
    for (int level = 4; level > 1; level--) {
        int index = NTH_LEVEL_INDEX(level, vaddr);
        if (!table[index]) {
            if (!attrs) {
                return NULL;
            }

            /* The PDPT, PD or PT is not allocated. Allocate it. */
            void *page = kmalloc(PAGE_SIZE);
            if (!page) {
                return NULL;
            }

            memset(page, 0, PAGE_SIZE);
            table[index] = (uint64_t) into_paddr(page);
        }

        // Update attributes if given.
        table[index] = table[index] | attrs;

        // Go into the next level paging table.
        table = (uint64_t *) from_paddr(ENTRY_PADDR(table[index]));
    }

    return &table[NTH_LEVEL_INDEX(1, vaddr)];
}

extern char __kernel_heap[];

static void free_page_table(const uint64_t *table, int level) {
    for (int i = 0; i < PAGE_ENTRY_NUM; i++) {
        paddr_t paddr = ENTRY_PADDR(table[i]);
        if (level > 1 && paddr && paddr >= (paddr_t) __kernel_heap) {
            free_page_table(from_paddr(paddr), level - 1);
        }
    }
}

error_t vm_create(struct vm *vm) {
    uint64_t *pml4 = kmalloc(PAGE_SIZE);
    if (!pml4) {
        return ERR_NO_MEMORY;
    }

    memcpy(pml4, from_paddr((paddr_t) __kernel_pml4), PAGE_SIZE);

    // The kernel no longer access a virtual address around 0x0000_0000. Unmap
    // the area to catch bugs (especially NULL pointer dereferences in the
    // kernel).
    pml4[0] = 0;

    vm->pml4 = into_paddr(pml4);
    return OK;
}

void vm_destroy(struct vm *vm) {
    free_page_table(from_paddr(vm->pml4), 4);
}

error_t vm_link(struct vm *vm, vaddr_t vaddr, paddr_t paddr,
                pageattrs_t attrs) {
    ASSERT(vaddr < KERNEL_BASE_ADDR && "tried to link a kernel page");
    ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));
    ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));

    attrs |= PAGE_PRESENT;
    uint64_t *entry = traverse_page_table(vm->pml4, vaddr, attrs);
    if (!entry) {
        return ERR_NO_MEMORY;
    }

    *entry = paddr | attrs;
    asm_invlpg(vaddr);
    return OK;
}

void vm_unlink(struct vm *vm, vaddr_t vaddr) {
    ASSERT(vaddr < KERNEL_BASE_ADDR && "tried to link a kernel page");
    ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));

    uint64_t *entry = traverse_page_table(vm->pml4, vaddr, 0);
    if (!entry) {
        return;
    }

    *entry = 0;
    asm_invlpg(vaddr);
}
