#include <kernel/arch.h>
#include <kernel/memory.h>
#include <kernel/printk.h>
#include "vm.h"

static pte_t page_attrs_to_pte_flags(unsigned attrs) {
    return
        (attrs & PAGE_READABLE) ? PTE_R : 0 |
        (attrs & PAGE_WRITABLE) ? PTE_W : 0 |
        (attrs & PAGE_EXECUTABLE) ? PTE_X : 0 |
        (attrs & PAGE_USER) ? PTE_U : 0;
}

static pte_t construct_pte(paddr_t paddr, pte_t flags) {
    return (paddr << PTE_PADDR_SHIFT) | flags;
}

static pte_t *walk(struct arch_vm *vm, vaddr_t vaddr, bool alloc) {
    ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));

    pte_t *table = arch_paddr2ptr(vm->table);
    for (int level = PAGE_TABLE_LEVELS - 1; level > 0; level--) {
        int index = PAGE_TABLE_INDEX(level, vaddr);
        if (table[index] == 0) {
            if (!alloc) {
                return NULL;
            }

            paddr_t paddr = pm_alloc(1, PAGE_TYPE_PAGE_TABLE, 0);
            table[index] = construct_pte(paddr, PTE_V);
        }
        table = arch_paddr2ptr(table[index]);
    }

    return &table[PAGE_TABLE_INDEX(0, vaddr)];
}

void arch_vm_init(struct arch_vm *vm) {
    vm->table = pm_alloc(1, PAGE_TYPE_PAGE_TABLE, 0);
}

error_t arch_vm_map(struct arch_vm *vm, vaddr_t vaddr, paddr_t paddr, unsigned attrs) {
    DEBUG_ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));

    pte_t *pte = walk(vm, vaddr, true);
    DEBUG_ASSERT(pte != NULL);

    if (*pte & PTE_V) {
        return ERR_EXISTS;
    }

    *pte = construct_pte(paddr, page_attrs_to_pte_flags(attrs) | PTE_V);
    return OK;
}
