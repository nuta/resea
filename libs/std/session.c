/// TODO: Consider kernel-level capability mechanism to allow users to delegate
/// handles. Refer "IPC Gate" concept in Fiasco L4 mickrokernel.
#include <list.h>
#include <std/malloc.h>
#include <std/session.h>

/// Whether this session library has been initialized.
static bool initialized = false;

/// Sessions. For simplicity, we use doubly-linked list while hash table is
/// way better choice.
static list_t sessions;

void session_init(void) {
    list_init(&sessions);
    initialized = true;
}

static handle_t alloc_handle_id(tid_t task) {
    for (handle_t handle = 1; handle < HANDLES_MAX; handle++) {
        bool duplicated = false;
        LIST_FOR_EACH (sess, &sessions, struct session, next) {
            if (sess->owner == task && sess->handle == handle) {
                duplicated = true;
                break;
            }
        }

        if (!duplicated) {
            return handle;
        }
    }

    return 0;
}

struct session *session_alloc(tid_t task) {
    DEBUG_ASSERT(initialized);

    handle_t handle = alloc_handle_id(task);
    if (!handle) {
        // Too many handles.
        return NULL;
    }

    return session_alloc_at(task, handle);
}

struct session *session_alloc_at(tid_t task, handle_t handle) {
    DEBUG_ASSERT(initialized);
    DEBUG_ASSERT(!session_get(task, handle));

    struct session *sess = malloc(sizeof(*sess));
    sess->owner = task;
    sess->handle = handle;
    sess->data = NULL;
    list_push_back(&sessions, &sess->next);
    return sess;
}

struct session *session_get(tid_t task, handle_t handle) {
    DEBUG_ASSERT(initialized);

    LIST_FOR_EACH (sess, &sessions, struct session, next) {
        if (sess->owner == task && sess->handle == handle) {
            return sess;
        }
    }

    return NULL;
}

struct session *session_delete(tid_t task, handle_t handle) {
    DEBUG_ASSERT(initialized);

    LIST_FOR_EACH (sess, &sessions, struct session, next) {
        if (sess->owner == task && sess->handle == handle) {
            list_remove(&sess->next);
            free(sess);
        }
    }

    return NULL;
}
