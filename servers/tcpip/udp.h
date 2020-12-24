#ifndef __UDP_H__
#define __UDP_H__

#include "device.h"
#include "mbuf.h"
#include "tcpip.h"
#include <list.h>

struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t checksum;
} __packed;

struct udp_datagram {
    list_elem_t next;
    ipaddr_t addr;
    port_t port;
    mbuf_t payload;
    device_t device;
};

#define UDP_SOCKETS_MAX 512
struct udp_socket {
    bool in_use;
    endpoint_t local;
    list_t rx;
    list_t tx;
    list_elem_t next;
};

typedef struct udp_socket *udp_sock_t;

udp_sock_t udp_new(void);
void udp_close(udp_sock_t sock);
void udp_bind(udp_sock_t sock, ipaddr_t *addr, port_t port);
void udp_sendto_mbuf(udp_sock_t sock, ipaddr_t *dst, port_t dst_port,
                     mbuf_t payload);
void udp_sendto(udp_sock_t sock, ipaddr_t *dst, port_t dst_port,
                const void *data, size_t len);
mbuf_t udp_recv_mbuf(udp_sock_t sock, device_t *device, ipaddr_t *src,
                     port_t *src_port);
size_t udp_recv(udp_sock_t sock, void *buf, size_t buf_len, device_t *device,
                ipaddr_t *src, port_t *src_port);
void udp_transmit(udp_sock_t sock);
void udp_receive(device_t device, ipaddr_t *src, mbuf_t pkt);
void udp_init(void);

#endif
