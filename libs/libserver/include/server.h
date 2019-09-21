#ifndef __SERVER_H__
#define __SERVER_H__

#include <types.h>

struct message;
error_t server_mainloop(cid_t ch, error_t (*process)(struct message *m));

#endif
