#ifndef __X64_VM_H__
#define __X64_VM_H__

#include <types.h>

#define NTH_LEVEL_INDEX(level, vaddr)                                          \
    (((vaddr) >> ((((level) -1) * 9) + 12)) & 0x1ff)
#define ENTRY_PADDR(entry) ((entry) &0x7ffffffffffff000)

#define X64_PF_PRESENT (1 << 0)
#define X64_PF_WRITE   (1 << 1)
#define X64_PF_USER    (1 << 2)

#define X64_PAGE_WRITABLE (1 << 1)

#endif
