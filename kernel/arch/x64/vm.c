#include <arch.h>
#include <printk.h>
#include <string.h>
#include "vm.h"

static uint64_t *traverse_page_table(uint64_t pml4, vaddr_t vaddr,
                                     paddr_t kpage, uint64_t attrs) {
    ASSERT(vaddr < KERNEL_BASE_ADDR);
    ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));
    ASSERT(IS_ALIGNED(kpage, PAGE_SIZE));

    uint64_t *table = from_paddr(pml4);
    for (int level = 4; level > 1; level--) {
        int index = NTH_LEVEL_INDEX(level, vaddr);
        if (!table[index]) {
            if (!attrs) {
                return NULL;
            }

            /* The PDPT, PD or PT is not allocated. */
            if (!kpage) {
                return NULL;
            }

            memset(from_paddr(kpage), 0, PAGE_SIZE);
            table[index] = kpage;
            kpage = 0;
        }

        // Update attributes if given.
        table[index] = table[index] | attrs;

        // Go into the next level paging table.
        table = (uint64_t *) from_paddr(ENTRY_PADDR(table[index]));
    }

    return &table[NTH_LEVEL_INDEX(1, vaddr)];
}

extern char __kernel_heap[];

error_t vm_create(struct vm *vm) {
    uint64_t *table = from_paddr(vm->pml4);
    memcpy(table, from_paddr((paddr_t) __kernel_pml4), PAGE_SIZE);

    // The kernel no longer access a virtual address around 0x0000_0000. Unmap
    // the area to catch bugs (especially NULL pointer dereferences in the
    // kernel).
    table[0] = 0;
    return OK;
}

void vm_destroy(struct vm *vm) {
}

error_t vm_link(struct vm *vm, vaddr_t vaddr, paddr_t paddr, paddr_t kpage,
                unsigned flags) {
    ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));
    uint64_t attrs = (1 << 2) | 1 /* user, present */;
    attrs |= (flags & MAP_W) ? X64_PAGE_WRITABLE : 0;

    uint64_t *entry = traverse_page_table(vm->pml4, vaddr, kpage, attrs);
    if (!entry) {
        return (kpage) ? ERR_TRY_AGAIN : ERR_EMPTY;
    }

    *entry = paddr | attrs;
    asm_invlpg(vaddr);
    return OK;
}

void vm_unlink(struct vm *vm, vaddr_t vaddr) {
    uint64_t *entry = traverse_page_table(vm->pml4, vaddr, 0, 0);
    if (entry) {
        *entry = 0;
        asm_invlpg(vaddr);
    }
}

paddr_t vm_resolve(struct vm *vm, vaddr_t vaddr) {
    uint64_t *entry = traverse_page_table(vm->pml4, vaddr, 0, 0);
    return (entry) ? ENTRY_PADDR(*entry) : 0;
}
