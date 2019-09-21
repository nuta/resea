#ifndef __SERVER_H__
#define __SERVER_H__

#include <types.h>

struct message;
error_t server_mainloop(cid_t ch, error_t (*process)(struct message *m));
error_t server_mainloop_with_deferred(cid_t ch,
                                      error_t (*process)(struct message *m),
                                      void (*deferred_work)(void));
error_t server_register(cid_t discovery_server, cid_t receive_at,
                        uint16_t interface, cid_t *new_ch);

#endif
