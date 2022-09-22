#include "vm.h"
#include "asm.h"
#include "plic.h"
#include "uart.h"
#include <kernel/arch.h>
#include <kernel/memory.h>
#include <kernel/printk.h>

static pte_t page_attrs_to_pte_flags(unsigned attrs) {
    return ((attrs & PAGE_READABLE) ? PTE_R : 0)
           | ((attrs & PAGE_WRITABLE) ? PTE_W : 0)
           | ((attrs & PAGE_EXECUTABLE) ? PTE_X : 0)
           | ((attrs & PAGE_USER) ? PTE_U : 0);
}

static pte_t construct_pte(paddr_t paddr, pte_t flags) {
    DEBUG_ASSERT((paddr & ~PTE_PADDR_MASK) == 0);
    return ((paddr >> 12) << 10) | flags;
}

static pte_t *walk(paddr_t root_table, vaddr_t vaddr, bool alloc) {
    ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));

    pte_t *table = arch_paddr2ptr(root_table);
    for (int level = PAGE_TABLE_LEVELS - 1; level > 0; level--) {
        int index = PTE_INDEX(level, vaddr);
        if (table[index] == 0) {
            if (!alloc) {
                return NULL;
            }

            paddr_t paddr =
                pm_alloc(PAGE_SIZE, PAGE_TYPE_KERNEL, PM_ALLOC_ZEROED);
            table[index] = construct_pte(paddr, PTE_V);  // TODO: paddr check
        }
        table = arch_paddr2ptr(PTE_PADDR(table[index]));
    }

    return &table[PTE_INDEX(0, vaddr)];
}

error_t arch_vm_map(struct arch_vm *vm, vaddr_t vaddr, paddr_t paddr,
                    size_t size, unsigned attrs) {
    DEBUG_ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));
    DEBUG_ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));
    DEBUG_ASSERT(IS_ALIGNED(size, PAGE_SIZE));

    // Check if pages are already mapped.
    for (uint32_t offset = 0; offset < size; offset += PAGE_SIZE) {
        pte_t *pte = walk(vm->table, vaddr + offset, true);
        if (*pte & PTE_V) {
            return ERR_ALREADY_EXISTS;
        }
    }

    // Map pages.
    for (uint32_t offset = 0; offset < size; offset += PAGE_SIZE) {
        pte_t *pte = walk(vm->table, vaddr + offset, true);
        DEBUG_ASSERT(pte != NULL);
        DEBUG_ASSERT((*pte & PTE_V) == 0);

        *pte = construct_pte(paddr + offset,
                             page_attrs_to_pte_flags(attrs) | PTE_V);
    }

    asm_sfence_vma();
    return OK;
}

error_t arch_vm_unmap(struct arch_vm *vm, vaddr_t vaddr, size_t size) {
    for (uint32_t offset = 0; offset < size; offset += PAGE_SIZE) {
        pte_t *pte = walk(vm->table, vaddr + offset, true);
        if ((*pte & PTE_V) == 0) {
            return ERR_NOT_FOUND;
        }

        paddr_t paddr = PTE_PADDR(*pte);
        *pte = 0;
        pm_free(paddr, PAGE_SIZE);
    }

    asm_sfence_vma();
    return OK;
}

bool riscv32_is_mapped(uint32_t satp, vaddr_t vaddr) {
    satp = (satp & 0x3fffffff) << 12;  // TODO:
    uint32_t *pte = walk(satp, ALIGN_DOWN(vaddr, PAGE_SIZE), false);
    return pte != NULL && (*pte & PTE_V);
}

error_t arch_vm_init(struct arch_vm *vm) {
    // TODO: Handle out of memory
    vm->table = pm_alloc(PAGE_SIZE, PAGE_TYPE_KERNEL, PM_ALLOC_ZEROED);

    paddr_t kernel_text = (paddr_t) __text;
    paddr_t kernel_text_end = (paddr_t) __text_end;

    DEBUG_ASSERT(IS_ALIGNED(kernel_text, PAGE_SIZE));
    DEBUG_ASSERT(IS_ALIGNED(kernel_text_end, PAGE_SIZE));

    paddr_t kernel_data = kernel_text_end;
    paddr_t kernel_data_size = 64 * 1024 * 1024;  // TODO: get from memory map
    size_t kernel_text_size = kernel_text_end - kernel_text;

    ASSERT_OK(arch_vm_map(vm, kernel_text, kernel_text, kernel_text_size,
                          PAGE_READABLE | PAGE_EXECUTABLE));
    ASSERT_OK(arch_vm_map(vm, kernel_data, kernel_data, kernel_data_size,
                          PAGE_READABLE | PAGE_WRITABLE));
    ASSERT_OK(arch_vm_map(vm, UART_ADDR, UART_ADDR, PAGE_SIZE,
                          PAGE_READABLE | PAGE_WRITABLE));
    ASSERT_OK(arch_vm_map(vm, PLIC_ADDR, PLIC_ADDR, PLIC_SIZE,
                          PAGE_READABLE | PAGE_WRITABLE));
    return OK;
}

void arch_vm_destroy(struct arch_vm *vm) {
    uint32_t *table0 = arch_paddr2ptr(vm->table);
    for (int i = 0; i < 1024; i++) {
        if (table0[i] & PTE_V) {
            paddr_t paddr = PTE_PADDR(table0[i]);
            pm_free(paddr, PAGE_SIZE);
        }
    }

    pm_free(vm->table, PAGE_SIZE);
}
