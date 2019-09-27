#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <types.h>
#include <arch.h>
#include <list.h>
#include <printk.h>

/// A memory pool.
struct arena {
    /// The memory pool buffer.
    vaddr_t elements;
    /// The size of the buffer.
    size_t element_size;
    /// The miximum number of elements.
    size_t elements_max;
    /// The number of allocated (in use) elements.
    size_t elements_used;
    /// The list of unused elements.
    struct list_head free_list;
};

/// Unused element.
struct freed_element {
    struct list_head next;
};

extern struct arena page_arena;
extern struct arena small_arena;

void *kmalloc_from(struct arena *arena);
void kfree(struct arena *arena, void *ptr);
paddr_t page_fault_handler(vaddr_t addr, uintmax_t flags);
void memory_init(void);

#define KMALLOC(arena, size)                                            \
    ({                                                                  \
        ASSERT(is_constant(size) && "Not a fixed-sized object!");       \
        ASSERT((size) <= (arena)->element_size && "Too large object!"); \
        kmalloc_from(arena);                                            \
    })

#endif
