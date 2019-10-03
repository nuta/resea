#ifndef __RESEA_IPC_H__
#define __RESEA_IPC_H__

#include <types.h>
#include <message.h>

struct message *get_ipc_buffer(void);
void set_page_base(page_base_t page_base);

#ifdef __x86_64__
#include <resea/x64.h>
#else
#    error "unsupported CPU architecture"
#endif

inline struct thread_info *get_thread_info(void);
inline void copy_to_ipc_buffer(const struct message *m);
inline void copy_from_ipc_buffer(struct message *buf);

inline void sys_nop(void);
inline int sys_open(void);
inline error_t sys_close(cid_t ch);
inline error_t sys_link(cid_t ch1, cid_t ch2);
inline error_t sys_transfer(cid_t src, cid_t dst);
inline error_t sys_ipc(cid_t ch, uint32_t ops);

error_t open(cid_t *ch);
error_t close(cid_t ch);
error_t link(cid_t ch1, cid_t ch2);
error_t transfer(cid_t src, cid_t dst);
error_t ipc_recv(cid_t ch, struct message *r);
error_t ipc_send(cid_t ch, struct message *m);
error_t ipc_async_send(cid_t ch, struct message *m);
error_t ipc_call(cid_t ch, struct message *m, struct message *r);

#endif
