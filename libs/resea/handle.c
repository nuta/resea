#include <resea/handle.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <list.h>

// The number of buckets in `handles`.
#define NUM_BUCKETS 32
// The maximum number of handles per task.
#define HANDLE_MAX  128

/// A hash table of handles.
static list_t *handles[NUM_BUCKETS];

struct handle_entry {
    list_elem_t next;
    task_t owner;
    void *data;
    handle_t handle;
};

static struct handle_entry *get_entry(task_t owner, handle_t handle) {
    unsigned index = handle % NUM_BUCKETS;
    list_t *list = handles[index];
    if (!list) {
        return NULL;
    }

    LIST_FOR_EACH (e, list, struct handle_entry, next) {
        if (e->owner == owner && e->handle == handle) {
            return e;
        }
    }

    return NULL;
}

handle_t handle_alloc(task_t owner) {
    // Look for an unused handle ID for the task.
    handle_t new_handle = 0;
    for (handle_t handle = 1; handle <= HANDLE_MAX; handle++) {
        if (!get_entry(owner, handle)) {
            new_handle = handle;
            break;
        }
    }

    if (!new_handle) {
        return ERR_NO_MEMORY;
    }

    // Allocate a bucket if needed.
    unsigned index = new_handle % NUM_BUCKETS;
    list_t *list = handles[index];
    if (!list) {
        list = malloc(sizeof(*list));
        list_init(list);
        handles[index] = list;
    }

    // Add a handle entry into the bucket.
    struct handle_entry *e = malloc(sizeof(*e));
    e->handle = new_handle;
    e->owner = owner;
    e->data = NULL;
    list_push_back(list, &e->next);
    return new_handle;
}

void *handle_get(task_t owner, handle_t handle) {
    struct handle_entry *e = get_entry(owner, handle);
    return e ? e->data : NULL;
}

void handle_set(task_t owner, handle_t handle, void *data) {
    struct handle_entry *e = get_entry(owner, handle);
    ASSERT(e);
    e->data = data;
}

void handle_free(task_t owner, handle_t handle) {
    struct handle_entry *e = get_entry(owner, handle);
    if (!e) {
        return;
    }

    list_remove(&e->next);
    free(e);
}
