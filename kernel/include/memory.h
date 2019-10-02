#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <types.h>
#include <arch.h>
#include <list.h>
#include <printk.h>

#define FREE_LIST_MAGIC1 0xdeaddead
#define FREE_LIST_MAGIC2 0xbadbadba

/// Unused element.
struct free_list {
    uint64_t magic1;
#ifdef DEBUG_BUILD
    // A unused space to detect use-after-free bug by ASan: `padding` is marked
    // as ASAN_FREED when the memory chunk is freed.
    uint8_t padding[200];
#endif
    struct list_head next;
    size_t num_objects;
    uint64_t magic2;
};

/// A memory pool.
struct kmalloc_arena {
    struct list_head free_list;
    size_t object_size;
    size_t num_objects;
};

struct vmarea;
typedef paddr_t (*pager_t)(struct vmarea *vma, vaddr_t vaddr);

/// A region in a virtual address space.
struct vmarea {
    /// The next vmarea in the process.
    struct list_head next;
    /// The start of the virtual memory region.
    vaddr_t start;
    /// The start of the virtual memory region.
    vaddr_t end;
    /// The pager function. A pager is responsible to fill pages within the
    /// vmarea and is called when a page fault has occurred.
    pager_t pager;
    /// The pager-specific data.
    void *arg;
    /// Allowed operations to the vmarea. Defined operations are:
    ///
    /// - PAGE_WRITABLE: The region is writable. If this flag is not set, the
    ///                  region is readonly.
    /// - PAGE_USER: The region is accessible from the user. If this flag is
    ///              not set, the region is for accessible only from the
    ///              kernel.
    ///
    uintmax_t flags;
};

extern struct kmalloc_arena page_arena;
extern struct kmalloc_arena object_arena;

void *kmalloc_from(struct kmalloc_arena *arena);
void kfree(struct kmalloc_arena *arena, void *ptr);
struct process;
error_t vmarea_create(struct process *process, vaddr_t start, vaddr_t end,
                      pager_t pager, void *pager_arg, int flags);
void vmarea_destroy(struct vmarea *vma);
paddr_t page_fault_handler(vaddr_t addr, uintmax_t flags);
void memory_init(void);

#define KMALLOC(arena, size)                                            \
    ({                                                                  \
        ASSERT(is_constant(size) && "Not a fixed-sized object!");       \
        ASSERT((size) <= (arena)->object_size && "Too large object!");  \
        kmalloc_from(arena);                                            \
    })

#endif
