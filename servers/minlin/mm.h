#ifndef __MM_H__
#define __MM_H__

#include <types.h>
#include <list.h>

struct mchunk {
    list_elem_t next;
    vaddr_t vaddr;
    paddr_t paddr;
    void *buf;
    size_t len;
};

struct mm {
    list_t mchunks;
};

struct proc;
error_t copy_from_user(struct proc *proc, void *dst, vaddr_t src,
                       size_t len);
error_t copy_to_user(struct proc *proc, vaddr_t dst, const void *src,
                     size_t len);
size_t strncpy_from_user(struct proc *proc, char *dst, vaddr_t src,
                         size_t max_len);
error_t handle_page_fault(struct proc *proc, vaddr_t vaddr, unsigned fault);
struct mchunk *mm_alloc_mchunk(struct mm *mm, vaddr_t vaddr, size_t len);
struct mchunk *mm_clone_mchunk(struct proc *child, struct mchunk *src);
errno_t mm_fork(struct proc *parent, struct proc *child);

#endif
