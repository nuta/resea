#ifndef __ARCH_TYPES_H__
#define __ARCH_TYPES_H__

typedef uint32_t pageattrs_t;
typedef uint32_t size_t;
typedef int32_t intmax_t;
typedef uint32_t uintmax_t;
typedef uint32_t paddr_t;
typedef uint32_t vaddr_t;
typedef uint32_t uintptr_t;

#define PAGE_SIZE     1024 // FIXME:
#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITABLE (1 << 1)
#define PAGE_USER     (1 << 2)

typedef uint32_t pagefault_t;
#define PF_PRESENT (1 << 0)
#define PF_WRITE   (1 << 1)
#define PF_USER    (1 << 2)

typedef struct {
} trap_frame_t;

#endif
