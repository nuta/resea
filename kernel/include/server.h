#ifndef __SERVER_H__
#define __SERVER_H__

#include <ipc.h>

#define INVALID_MESSAGE         0x0001
#define PUTCHAR_INTERFACE       0x04
#define PUTCHAR_REQUEST         0x0401
#define PUTCHAR_RESPONSE        0x0481
#define CREATE_PROCESS_REQUEST  0x0102
#define CREATE_PROCESS_RESPONSE 0x0182
#define SPAWN_THREAD_REQUEST    0x0103
#define SPAWN_THREAD_RESPONSE   0x0183
#define ADD_PAGER_REQUEST       0x0104
#define ADD_PAGER_RESPONSE      0x0184
#define POWEROFF_REQUEST        0x0105
#define FILL_PAGE_REQUEST       0x0301

error_t kernel_server(payload_t header,
                      payload_t p0,
                      payload_t p1,
                      payload_t p2,
                      payload_t p3,
                      payload_t p4,
                      struct thread *client);

#endif
