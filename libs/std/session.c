#include <std/malloc.h>
#include <std/session.h>

struct session {
    bool in_use;
    handle_t handle;
    void *data;
};

/// Whether this session library has been initialized.
static bool initialized = false;

/// Sessions. For simplicity, we use doubly-linked list while hash table is
/// way better choice.
static struct session sessions[SESSIONS_MAX];

static struct session *alloc(void) {
    for (int i = 0; i < SESSIONS_MAX; i++) {
        if (!sessions[i].in_use) {
            sessions[i].in_use = true;
            return &sessions[i];
        }
    }

    return NULL;
}

struct session *lookup(handle_t handle) {
    DEBUG_ASSERT(initialized);

    for (int i = 0; i < SESSIONS_MAX; i++) {
        if (sessions[i].in_use && sessions[i].handle == handle) {
            return &sessions[i];
        }
    }

    return NULL;
}

void session_init(void) {
    initialized = true;
    for (int i = 0; i < SESSIONS_MAX; i++) {
        sessions[i].in_use = false;
        sessions[i].handle = i + 1;
    }
}

handle_t session_new(void) {
    DEBUG_ASSERT(initialized);

    struct session *sess = alloc();
    ASSERT(sess);
    return sess->handle;
}

error_t session_alloc(handle_t handle) {
    DEBUG_ASSERT(initialized);

    if (lookup(handle)) {
        return ERR_ALREADY_EXISTS;
    }

    struct session *sess = alloc();
    sess->handle = handle;
    return OK;
}

void session_delete(handle_t handle) {
    // TODO:
}

void *session_get(handle_t handle) {
    DEBUG_ASSERT(initialized);

    struct session *sess = lookup(handle);
    if (!sess) {
        return NULL;
    }

    return sess->data;
}

void session_set(handle_t handle, void *data) {
    DEBUG_ASSERT(initialized);

    struct session *sess = lookup(handle);
    ASSERT(sess);
    sess->data = data;
}
