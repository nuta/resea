#include "shm.h"
#include "page_alloc.h"
#include "page_fault.h"
#include "task.h"
#include <list.h>
#include <resea/malloc.h>
#include <string.h>

static list_t mappings;

static handle_t into_shm_handle(task_t owner_tid, vaddr_t owner_vaddr) {
    DEBUG_ASSERT(owner_tid < PAGE_SIZE);
    DEBUG_ASSERT(IS_ALIGNED(owner_vaddr, PAGE_SIZE));
    return owner_vaddr | owner_tid;
}

static error_t from_shm_handle(handle_t handle, task_t *owner_tid,
                               vaddr_t *owner_vaddr) {
    *owner_vaddr = handle & ~(PAGE_SIZE - 1);
    *owner_tid = handle & (PAGE_SIZE - 1);

    bool valid =
        (*owner_tid < PAGE_SIZE) && IS_ALIGNED(*owner_vaddr, PAGE_SIZE);
    if (!valid) {
        return ERR_INVALID_ARG;
    }
    return OK;
}

static struct shm_mapping *lookup(struct task *task, vaddr_t vaddr) {
    LIST_FOR_EACH (m, &mappings, struct shm_mapping, next) {
        if (m->task == task && m->vaddr == vaddr) {
            return m;
        }
    }

    return NULL;
}

error_t shm_create(struct task *task, size_t size, handle_t *handle,
                   vaddr_t *vaddr) {
    size = ALIGN_UP(size, PAGE_SIZE);

    paddr_t paddr = 0;
    error_t err = task_page_alloc(task, vaddr, &paddr, size);
    if (err != OK) {
        return err;
    }

    struct shm *shm = malloc(sizeof(*shm));
    shm->size = size;
    shm->paddr = paddr;
    shm->ref_count = 1;

    struct shm_mapping *m = malloc(sizeof(*m));
    m->shm = shm;
    m->task = task;
    m->vaddr = *vaddr;
    list_push_back(&mappings, &m->next);

    *handle = into_shm_handle(task->tid, *vaddr);
    return OK;
}

error_t shm_map(handle_t handle, struct task *task, bool writable,
                vaddr_t *vaddr) {
    task_t owner_tid;
    vaddr_t vaddr_in_owner;
    error_t err;
    if ((err = from_shm_handle(handle, &owner_tid, &vaddr_in_owner)) != OK) {
        return err;
    }

    struct task *owner = task_lookup(owner_tid);
    if (!owner) {
        return ERR_NOT_FOUND;
    }

    struct shm_mapping *owner_m = lookup(owner, vaddr_in_owner);
    if (!owner_m) {
        return ERR_NOT_FOUND;
    }

    struct shm *shm = owner_m->shm;
    int flag = (writable) ? MAP_TYPE_READWRITE : MAP_TYPE_READONLY;

    *vaddr = virt_page_alloc(task, shm->size);
    err = map_page(task, *vaddr, shm->paddr, flag, true);
    if (err != OK) {
        return err;
    }

    struct shm_mapping *m = malloc(sizeof(*m));
    m->shm = shm;
    m->task = task;
    m->vaddr = *vaddr;
    list_push_back(&mappings, &m->next);

    return OK;
}

void shm_close(struct task *task, vaddr_t vaddr) {
    struct shm_mapping *m = lookup(task, vaddr);
    if (!m) {
        return;
    }

    struct shm *shm = m->shm;
    DEBUG_ASSERT(shm->ref_count > 0);

    m->shm->ref_count--;
    if (m->shm->ref_count == 0) {
        // The shared memory area is no longer used. Free it.
        DEBUG_ASSERT(IS_ALIGNED(m->shm->size, PAGE_SIZE));

        for (offset_t off = 0; off < m->shm->size; off += PAGE_SIZE) {
            task_page_free(task, m->shm->paddr + off);
        }

        free(m->shm);
    }

    free(m);
    list_remove(&m->next);
}

void shm_init(void) {
    list_init(&mappings);
}
