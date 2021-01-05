#ifndef __SHM_H__
#define __SHM_H__

#include <types.h>
#include "task.h"

struct shm {
    bool inuse;
    int shm_id;
    paddr_t paddr;
    size_t len;
};

#define NUM_SHARED_MEMS_MAX 32
int shm_check_available(void);
error_t shm_create(struct task* task, size_t size, int* slot);
error_t shm_map(struct task* task, int shm_id, bool writable,vaddr_t *vaddr);
void shm_close(int shm_id);
struct shm *shm_lookup(int shm_id);
#endif
