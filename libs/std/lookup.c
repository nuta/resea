#include <message.h>
#include <std/syscall.h>
#include <cstring.h>

task_t ipc_lookup(const char *name) {
    struct ipc_msg_t m;
    m.type = LOOKUP_MSG;
    strncpy(m.lookup.name, name, sizeof(m.lookup.name));

    error_t err = ipc_call(INIT_TASK_TID, &m);
    if (IS_ERROR(err)) {
        return err;
    }

    return m.lookup_reply.task;
}
