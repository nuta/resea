#ifndef __STUBS_TCPIP_H__
#define __STUBS_TCPIP_H__

#include <types.h>

error_t tcpip_init(void);
task_t tcpip_server(void);
handle_t tcpip_listen(uint16_t port, int backlog);
handle_t tcpip_accept(handle_t handle);
error_t tcpip_write(handle_t handle, const void *data, size_t len);
error_t tcpip_read(handle_t handle, void *buf, size_t buf_len);
error_t tcpip_close(handle_t handle);

#endif
