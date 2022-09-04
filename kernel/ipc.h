#pragma once

#define IPC_ANY  -1
#define IPC_DENY -2

#define IPC_SEND    (1 << 16)
#define IPC_RECV    (1 << 17)
#define IPC_NOBLOCK (1 << 18)
#define IPC_KERNEL  (1 << 19)

#define NOTIFY_ABORTED (1 << 0)
