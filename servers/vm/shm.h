#ifndef __SHM_H__
#define __SHM_H__

#include <list.h>
#include <types.h>

struct shm {
    int ref_count;
    paddr_t paddr;
    size_t size;
};

struct task;

struct shm_mapping {
    struct shm *shm;
    list_elem_t next;
    struct task *task;
    vaddr_t vaddr;
};

error_t shm_create(struct task *task, size_t size, vaddr_t *vaddr);
error_t shm_map(struct task *task, struct task *owner, vaddr_t vaddr_in_owner,
                bool writable, vaddr_t *vaddr);
void shm_close(struct task *task, vaddr_t vaddr);
void shm_init(void);

#endif
