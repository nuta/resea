#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <types.h>
#include <message.h>

// A bit mask to determine if a message satisfies one of fastpath
// prerequisites. This test checks if page/channel payloads are
// not contained in the message.
#define SYSCALL_FASTPATH_TEST(header) ((header) & 0x1800ULL)

cid_t sys_open(void);
error_t sys_close(cid_t cid);
error_t sys_ipc(cid_t cid, uint32_t syscall);
error_t sys_notify(cid_t cid, notification_t notification);
intmax_t syscall_handler(uintmax_t arg0, uintmax_t arg1, uintmax_t syscall);

#endif
