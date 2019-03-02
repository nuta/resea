#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <types.h>

void *alloc_page(void);
void *alloc_object(void);
void free_page(void *ptr);
void free_object(void *ptr);
void page_fault_handler(vaddr_t vaddr, uintmax_t error);
void memory_init(void);

#endif
