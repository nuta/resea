#include "shm.h"
#include <string.h>

int search(int shm_id) {
    int i = 0;
    for (; i < NUM_SHARED_MEMS_MAX; i++) {
        if (shared_mems[i].shm_id == shm_id)
            return i;
    }
    return -1;
}
int shm_check_available() {
    int i = 0;
    for (; i < NUM_SHARED_MEMS_MAX && shared_mems[i].inuse; i++)
        ;

    if (i == NUM_SHARED_MEMS_MAX)
        return -1;
    return i;
}
int shm_create(size_t size, paddr_t paddr,int slot) {
    shared_mems[slot].inuse = true;
    shared_mems[slot].shm_id = slot;
    shared_mems[slot].len = size;
    shared_mems[slot].paddr = paddr;
    return slot;
}
int shm_close(int shm_id) {
    int i = search(shm_id);
    if (i >= 0){
        bzero(&shared_mems[i], sizeof(struct shm_t));
        return shm_id;
    }
    return -1;  // SHM not open
}

struct shm_t* shm_stat(int shm_id) {
    int i = 0;
    for (; i < NUM_SHARED_MEMS_MAX; i++) {
        if (shared_mems[i].shm_id == shm_id) {
            if (shared_mems[i].inuse == false) {
                return NULL;
            }
            return &shared_mems[i];
        }
    }
    return NULL;
}
