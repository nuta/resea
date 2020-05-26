#include <resea/printf.h>
#include <resea/handle.h>

static void *handles[HANDLES_MAX];

void handle_init(void) {
    for (size_t i = 0; i < HANDLES_MAX; i++) {
        handles[i] = NULL;
    }
}

handle_t handle_alloc(void) {
    for (size_t i = 0; i < HANDLES_MAX; i++) {
        if (handles[i] == NULL) {
            return i + 1;
        }
    }

    return ERR_NO_MEMORY;
}

void *handle_get(handle_t handle) {
    if (handle <= 0 || handle > HANDLES_MAX) {
        return NULL;
    }

    return handles[handle - 1];
}

void handle_set(handle_t handle, void *data) {
    ASSERT(0 < handle && handle <= HANDLES_MAX);
    ASSERT(data);
    handles[handle - 1] = data;
}

void handle_free(handle_t handle) {
    ASSERT(0 < handle && handle <= HANDLES_MAX);
    handles[handle - 1] = NULL;
}

