#ifndef __X64_VM_H__
#define __X64_VM_H__

#include <types.h>

#define NTH_LEVEL_INDEX(level, vaddr)                                          \
    (((vaddr) >> ((((level) -1) * 9) + 12)) & 0x1ff)
#define ENTRY_PADDR(entry) ((entry) &0x7ffffffffffff000)

#endif
