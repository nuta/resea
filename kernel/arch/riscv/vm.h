#pragma once
#include <types.h>

#define PAGE_TABLE_LEVELS 2
#define PAGE_TABLE_INDEX(level, vaddr) (((vaddr) >> (12 + (level) * 10)) & 0x3ff)
#define PTE_PADDR_SHIFT 10

#define PTE_V (1 << 0)
#define PTE_R (1 << 1)
#define PTE_W (1 << 2)
#define PTE_X (1 << 3)
#define PTE_U (1 << 4)

typedef uint32_t pte_t;
