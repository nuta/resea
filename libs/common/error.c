#include <error.h>

static const char *error_names[] = {
    [-OK] = "OK",
    [-ERR_NO_MEMORY] = "No Memory",
    [-ERR_NO_RESOURCES] = "No Resources",
    [-ERR_NOT_FOUND] = "Not Found",
    [-ERR_ALREADY_EXISTS] = "Already Exists",
    [-ERR_ALREADY_USED] = "Already Used",
    [-ERR_STILL_USED] = "Still Used",
    [-ERR_NOT_ALLOWED] = "Not Allowed",
    [-ERR_NOT_SUPPORTED] = "Not Supported",
    [-ERR_INVALID_ARG] = "Invalid Argument",
    [-ERR_INVALID_TASK] = "Invalid Task",
    [-ERR_INVALID_SYSCALL] = "Invalid Syscall",
    [-ERR_TOO_MANY_TASKS] = "Too Many Tasks",
    [-ERR_TOO_LARGE] = "Too Large",
    [-ERR_TOO_SMALL] = "Too Small",
    [-ERR_WOULD_BLOCK] = "Would Block",
    [-ERR_ABORTED] = "Aborted",
};

const char *err2str(int err) {
    if (err < ERR_END || err > 0) {
        return "(Unknown Error)";
    }

    return error_names[-err];
}
