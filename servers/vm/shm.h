#ifndef __SHM_H__
#define __SHM_H__

#include <types.h>
struct shm_t {
    bool inuse;
    int shm_id;
    paddr_t paddr;
    size_t len;
};

#define NUM_SHARED_MEMS_MAX 32
static struct shm_t shared_mems[NUM_SHARED_MEMS_MAX];

int shm_create(size_t size, paddr_t paddr);
vaddr_t shm_map(task_t tid, int shm_id);
int shm_close(int shm_id);
struct shm_t *shm_stat(int shm_id);
#endif 