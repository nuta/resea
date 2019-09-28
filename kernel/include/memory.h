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
    struct list_head next;
    size_t num_objects;
    uint64_t magic2;
};

/// A memory pool.
struct kmalloc_arena {
    struct list_head free_list;
    size_t object_size;
};

extern struct kmalloc_arena page_arena;
extern struct kmalloc_arena small_arena;

void *kmalloc_from(struct kmalloc_arena *arena);
void kfree(struct kmalloc_arena *arena, void *ptr);
paddr_t page_fault_handler(vaddr_t addr, uintmax_t flags);
void memory_init(void);

#define KMALLOC(arena, size)                                            \
    ({                                                                  \
        ASSERT(is_constant(size) && "Not a fixed-sized object!");       \
        ASSERT((size) <= (arena)->object_size && "Too large object!");  \
        kmalloc_from(arena);                                            \
    })

#endif
