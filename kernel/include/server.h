#ifndef __SERVER_H__
#define __SERVER_H__

#include <ipc.h>

#define INVALID_MESSAGE                0x0001
#define PUTCHAR_INTERFACE              0x03
#define PUTCHAR_REQUEST                0x0301
#define PUTCHAR_RESPONSE               0x0381
#define CREATE_PROCESS_REQUEST         0x0102
#define CREATE_PROCESS_RESPONSE        0x0182
#define SPAWN_THREAD_REQUEST           0x0103
#define SPAWN_THREAD_RESPONSE          0x0183
#define ADD_PAGER_REQUEST              0x0104
#define ADD_PAGER_RESPONSE             0x0184
#define ALLOW_IO_REQUEST               0x0105
#define ALLOW_IO_RESPONSE              0x0185
#define CREATE_KERNEL_CHANNEL_REQUEST  0x0106
#define CREATE_KERNEL_CHANNEL_RESPONSE 0x0186
#define CREATE_THREAD_REQUEST          0x0107
#define CREATE_THREAD_RESPONSE         0x0187
#define START_THREAD_REQUEST           0x0108
#define START_THREAD_RESPONSE          0x0188
#define FILL_PAGE_REQUEST              0x0201

error_t kernel_server(void);

#endif
