#include <message.h>

IPCSTUB_STATIC_ASSERTIONS

const char *msgtype2str(int type) {
    int id = MSG_ID(type);
    if (id == 0 || id > IPCSTUB_MSGID_MAX) {
        return "(invalid)";
    }

    return IPCSTUB_MSGID2STR[id];
}
