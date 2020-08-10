#ifndef __OOL_H__
#define __OOL_H__

#include <types.h>

struct message;
error_t handle_ool_recv(struct message *m);
error_t handle_ool_send(struct message *m);
error_t handle_ool_verify(struct message *m);

#endif
