#ifndef __ARM64_VM_H__
#define __ARM64_VM_H__

#define NTH_LEVEL_INDEX(level, vaddr)                                          \
    (((vaddr) >> ((((level) -1) * 9) + 12)) & 0x1ff)
#define ENTRY_PADDR(entry) ((entry) &0x0000fffffffff000)

#define ARM64_PAGE_TABLE  0x3
#define ARM64_PAGE_ACCESS (1ULL << 10)
// Readonly from both kernel and user.
#define ARM64_PAGE_MEMATTR_READONLY (0b11 << 6)
// Readable/writable from both kernel and user.
#define ARM64_PAGE_MEMATTR_READWRITE (0b01 << 6)

#endif
