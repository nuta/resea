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

extern char __kernel_text[];
extern char __kernel_text_end[];
extern char __kernel_data[];
extern char __kernel_data_end[];
extern char __kernel_image_end[];
extern char __boot_elf[];
