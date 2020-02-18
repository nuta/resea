#ifndef __ARCH_TYPES_H__
#define __ARCH_TYPES_H__

typedef uint64_t pageattrs_t;
typedef uint64_t size_t;
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;
typedef uint64_t paddr_t;
typedef uint64_t vaddr_t;
typedef uint64_t uintptr_t;

#define PAGE_SIZE     4096
#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITABLE (1 << 1)
#define PAGE_USER     (1 << 2)

typedef uint64_t pagefault_t;
#define PF_PRESENT (1 << 0)
#define PF_WRITE   (1 << 1)
#define PF_USER    (1 << 2)

#endif
