#include <list.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>
#include "device.h"
#include <endian.h>
#include "ipv4.h"
#include "udp.h"

static struct udp_socket sockets[UDP_SOCKETS_MAX];
static list_t active_socks;

static struct udp_socket *udp_lookup(port_t dst_port) {
    // Look for the listening socket at `dst_ep`.
    LIST_FOR_EACH (sock, &active_socks, struct udp_socket, next) {
        if (sock->local.port == dst_port) {
            return sock;
        }
    }

    return NULL;
}

udp_sock_t udp_new(void) {
    struct udp_socket *sock = NULL;
    for (int i = 0; i < UDP_SOCKETS_MAX; i++) {
        if (!sockets[i].in_use) {
            sock = &sockets[i];
            break;
        }
    }

    if (!sock) {
        return NULL;
    }

    sock->in_use = true;
    memset(&sock->local.addr, 0, sizeof(ipaddr_t));
    sock->local.port = 0;
    list_init(&sock->rx);
    list_init(&sock->tx);
    list_nullify(&sock->next);
    return sock;
}

void udp_close(udp_sock_t sock) {
    list_remove(&sock->next);
    sock->in_use = false;
}

void udp_bind(udp_sock_t sock, ipaddr_t *addr, port_t port) {
    ipaddr_copy(&sock->local.addr, addr);
    sock->local.port = port;
    list_push_back(&active_socks, &sock->next);
}

void udp_sendto_mbuf(udp_sock_t sock, ipaddr_t *dst, port_t dst_port,
                     mbuf_t payload) {
    struct udp_datagram *dg = (struct udp_datagram *) malloc(sizeof(*dg));
    memcpy(&dg->addr, dst, sizeof(ipaddr_t));
    dg->port = dst_port;
    dg->payload = payload;
    list_push_back(&sock->tx, &dg->next);
}

void udp_sendto(udp_sock_t sock, ipaddr_t *dst, port_t dst_port,
                const void *data, size_t len) {
    return udp_sendto_mbuf(sock, dst, dst_port, mbuf_new(data, len));
}

mbuf_t udp_recv_mbuf(udp_sock_t sock, device_t *device, ipaddr_t *src,
                     port_t *src_port) {
    list_elem_t *e = list_pop_front(&sock->rx);
    if (!e) {
        return NULL;
    }

    struct udp_datagram *dg = LIST_CONTAINER(e, struct udp_datagram, next);
    mbuf_t payload = dg->payload;
    ipaddr_copy(src, &dg->addr);
    *src_port = dg->port;
    *device = dg->device;
    free(dg);
    return payload;
}

size_t udp_recv(udp_sock_t sock, void *buf, size_t buf_len, device_t *device,
                ipaddr_t *src, port_t *src_port) {
    mbuf_t payload = udp_recv_mbuf(sock, device, src, src_port);
    if (!payload) {
        return 0;
    }

    size_t len = mbuf_len(payload);
    mbuf_read(&payload, buf, buf_len);
    mbuf_delete(payload);
    return len;
}

void udp_receive(device_t device, ipaddr_t *src, mbuf_t pkt) {
    struct udp_header header;
    if (mbuf_read(&pkt, &header, sizeof(header)) != sizeof(header)) {
        return;
    }

    uint16_t dst_port = ntoh16(header.dst_port);
    struct udp_socket *sock = udp_lookup(dst_port);
    if (!sock) {
        return;
    }

    // Enqueue the received datagram.
    struct udp_datagram *dg = (struct udp_datagram *) malloc(sizeof(*dg));
    memcpy(&dg->addr, src, sizeof(ipaddr_t));
    dg->device = device;
    dg->port = dst_port;
    dg->payload = pkt;
    list_push_back(&sock->rx, &dg->next);
}

void udp_transmit(udp_sock_t sock) {
    list_elem_t *e = list_pop_front(&sock->tx);
    if (!e) {
        return;
    }

    struct udp_datagram *dg = LIST_CONTAINER(e, struct udp_datagram, next);
    struct udp_header header;
    header.dst_port = hton16(dg->port);
    header.src_port = hton16(sock->local.port);
    header.checksum = 0;
    header.len = hton16(sizeof(header) + mbuf_len(dg->payload));
    mbuf_t pkt = mbuf_new(&header, sizeof(header));
    mbuf_prepend(dg->payload, pkt);

    switch (dg->addr.type) {
        case IP_TYPE_V4:
            ipv4_transmit(dg->addr.v4, IPV4_PROTO_UDP, pkt);
            break;
    }
}

void udp_init(void) {
    list_init(&active_socks);
    for (int i = 0; i < UDP_SOCKETS_MAX; i++) {
        sockets[i].in_use = false;
    }
}
