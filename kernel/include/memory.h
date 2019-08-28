#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <types.h>

struct arena;
extern struct arena page_arena;
extern struct arena small_arena;

void *kmalloc(struct arena *arena);
void kfree(struct arena *arena, void *ptr);
paddr_t page_fault_handler(vaddr_t addr, uintmax_t flags);
void memory_init(void);

#endif
