#include <string.h>
#include "mm.h"
#include "shm.h"

static struct shm shared_mems[NUM_SHARED_MEMS_MAX];

int shm_check_available() {
    int i;
    for (i = 0; i < NUM_SHARED_MEMS_MAX && shared_mems[i].inuse; i++)
        ;
    if (i == NUM_SHARED_MEMS_MAX)
        return -1;
    return i;
}

error_t shm_create(struct task* task, size_t size, int* slot) {
    *slot = shm_check_available();
    if (slot < 0) {
        return ERR_UNAVAILABLE;
    }
    paddr_t paddr;
    vaddr_t vaddr;
    error_t err = alloc_phy_pages(task, &vaddr, &paddr, size);
    if (err != OK) {
        return err;
    }
    shared_mems[*slot].inuse = true;
    shared_mems[*slot].shm_id = *slot;
    shared_mems[*slot].len = size;
    shared_mems[*slot].paddr = paddr;
    return OK;
}

error_t shm_map(struct task* task, int shm_id, bool writable, vaddr_t* vaddr) {
    struct shm* shm = shm_lookup(shm_id);
    if (shm == NULL) {
        return ERR_NOT_FOUND;
    }
    *vaddr = alloc_virt_pages(task, shm->len);
    int flag = (writable) ? MAP_W : 0;
    error_t err = map_page(task, *vaddr, shm->paddr, flag, true);
    return err;
}

void shm_close(int shm_id) {
    struct shm* shm = shm_lookup(shm_id);
    if (shm != NULL) {
        shm->inuse = false;
    }
}

struct shm* shm_lookup(int shm_id) {
    int i;
    for (i = 0; i < NUM_SHARED_MEMS_MAX; i++) {
        if (shared_mems[i].shm_id == shm_id) {
            if (shared_mems[i].inuse == false) {
                return NULL;
            }
            return &shared_mems[i];
        }
    }
    return NULL;
}
