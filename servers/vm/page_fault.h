#ifndef __PAGE_FAULT_H__
#define __PAGE_FAULT_H__

#include <types.h>

struct task;
error_t map_page(struct task *task, vaddr_t vaddr, paddr_t paddr,
                 unsigned flags, bool overwrite);
paddr_t handle_page_fault(struct task *task, vaddr_t vaddr, vaddr_t ip,
                          unsigned fault);
void page_fault_init(void);

#endif
