#ifndef __MM_H__
#define __MM_H__

#include <types.h>
#include <list.h>

struct page_area {
    list_elem_t next;
    vaddr_t vaddr;
    paddr_t paddr;
    size_t num_pages;
};

struct task;
error_t alloc_phy_pages(struct task *task, vaddr_t *vaddr, paddr_t *paddr,
                        size_t num_pages);
void free_page_area(struct page_area *area);
paddr_t vaddr2paddr(struct task *task, vaddr_t vaddr);
error_t map_page(struct task *task, vaddr_t vaddr, paddr_t paddr, unsigned flags,
                 bool overwrite);
paddr_t pager(struct task *task, vaddr_t vaddr, unsigned fault);

#endif
