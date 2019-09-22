#ifndef __IPC_H__
#define __IPC_H__

#include <types.h>
#include <message.h>

// A bit mask to determine if a message satisfies one of fastpath
// prerequisites. This test checks if page/channel payloads are
// not contained in the message.
#define SYSCALL_FASTPATH_TEST(header) ((header) & 0x1800ull)

struct process;
struct channel;
struct thread_info *get_thread_info(void);
struct channel *channel_create(struct process *process);
void channel_incref(struct channel *ch);
void channel_destroy(struct channel *ch);
void channel_link(struct channel *ch1, struct channel *ch2);
void channel_transfer(struct channel *src, struct channel *dst);
error_t channel_notify(struct channel *ch, notification_t notification);
cid_t sys_open(void);
error_t sys_close(cid_t cid);
error_t sys_ipc(cid_t cid, uint32_t ops);
error_t sys_notify(cid_t ch, notification_t notification);
intmax_t syscall_handler(uintmax_t arg0, uintmax_t arg1, uintmax_t syscall);
#endif
