#include <message.h>
#include <std/syscall.h>
#include <string.h>

tid_t ipc_lookup(const char *name) {
    struct message m;
    m.type = LOOKUP_MSG;
    strncpy(m.lookup.name, name, sizeof(m.lookup.name));

    error_t err = ipc_call(INIT_TASK_TID, &m, sizeof(m), &m, sizeof(m));
    if (IS_ERROR(err)) {
        return err;
    }

    return m.lookup_reply.task;
}
