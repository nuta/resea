#pragma once
#include <types.h>

#define PAGE_TABLE_LEVELS       2
#define PTE_PADDR_MASK          0xfffffc00
#define PTE_INDEX(level, vaddr) (((vaddr) >> (12 + (level) *10)) & 0x3ff)
#define PTE_PADDR(pte)          (((pte) >> 10) << 12)

#define PTE_V (1 << 0)
#define PTE_R (1 << 1)
#define PTE_W (1 << 2)
#define PTE_X (1 << 3)
#define PTE_U (1 << 4)

typedef uint32_t pte_t;

extern char __text[];
extern char __text_end[];
extern char __data[];
extern char __data_end[];
extern char __bss[];
extern char __bss_end[];
extern char __image_end[];
extern char __boot_elf[];

bool riscv32_is_mapped(uint32_t satp, vaddr_t vaddr);
