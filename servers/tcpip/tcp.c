#include <list.h>
#include <resea/printf.h>
#include <string.h>
#include "checksum.h"
#include "device.h"
#include <endian.h>
#include "ipv4.h"
#include "stats.h"
#include "sys.h"
#include "tcp.h"

static struct tcp_socket sockets[TCP_SOCKETS_MAX];
static list_t active_socks;

static void tcp_set_pendings(struct tcp_socket *sock, uint32_t pendings) {
    sock->pendings |= pendings;
}

static uint32_t tcp_clear_pendings(struct tcp_socket *sock) {
    uint32_t pendings = sock->pendings;
    sock->pendings = 0;
    return pendings;
}

static struct tcp_socket *tcp_lookup_local(endpoint_t *ep) {
    LIST_FOR_EACH (sock, &active_socks, struct tcp_socket, next) {
        if (!ipaddr_is_unspecified(&sock->local.addr)
            && !ipaddr_equals(&sock->local.addr, &ep->addr)) {
            continue;
        }

        if (sock->local.port != ep->port) {
            continue;
        }

        if (!ipaddr_is_unspecified(&sock->remote.addr)) {
            continue;
        }

        return sock;
    }

    return NULL;
}

static struct tcp_socket *tcp_lookup(endpoint_t *dst_ep, endpoint_t *src_ep) {
    // Look for the destination socket.
    LIST_FOR_EACH (sock, &active_socks, struct tcp_socket, next) {
        if (!ipaddr_is_unspecified(&sock->local.addr)
            && !ipaddr_equals(&sock->local.addr, &dst_ep->addr)) {
            continue;
        }

        if (sock->local.port != dst_ep->port) {
            continue;
        }

        if (!ipaddr_equals(&sock->remote.addr, &src_ep->addr)) {
            continue;
        }

        if (sock->remote.port != src_ep->port) {
            continue;
        }

        return sock;
    }

    // Look for the listening socket at `dst_ep`.
    return tcp_lookup_local(dst_ep);
}

tcp_sock_t tcp_new(void) {
    struct tcp_socket *sock = NULL;
    for (int i = 0; i < TCP_SOCKETS_MAX; i++) {
        if (!sockets[i].in_use) {
            sock = &sockets[i];
            break;
        }
    }

    if (!sock) {
        return NULL;
    }

    sock->in_use = true;
    sock->state = TCP_STATE_CLOSED;
    sock->pendings = 0;
    sock->next_seqno = 0;
    sock->last_seqno = 0;
    sock->last_ack = 0;
    sock->local_winsize = TCP_RX_BUF_SIZE;
    memset(&sock->local.addr, 0, sizeof(ipaddr_t));
    memset(&sock->remote.addr, 0, sizeof(ipaddr_t));
    sock->local.port = 0;
    sock->remote.port = 0;
    sock->rx_buf = mbuf_alloc();
    sock->tx_buf = mbuf_alloc();
    sock->retransmit_at = 0;
    sock->num_retransmits = 0;
    sock->backlog = 0;
    sock->listen_sock = NULL;
    list_init(&sock->backlog_socks);
    list_nullify(&sock->next);
    list_nullify(&sock->backlog_next);
    return sock;
}

void tcp_close(tcp_sock_t sock) {
    mbuf_delete(sock->rx_buf);
    mbuf_delete(sock->tx_buf);
    list_remove(&sock->next);
    sock->in_use = false;
}

void tcp_bind(tcp_sock_t sock, ipaddr_t *addr, port_t port) {
    endpoint_t ep;
    memcpy(&ep.addr, addr, sizeof(*addr));
    ep.port = port;

    // Check if there's a socket which is already binded to the specified
    // endpoint.
    if (tcp_lookup_local(&ep) != NULL) {
        WARN("already listening on %d", port);
        return;
    }

    memcpy(&sock->local, &ep, sizeof(sock->local));
}

void tcp_connect(tcp_sock_t sock, ipaddr_t *dst_addr, port_t dst_port) {
    for (port_t port = 1000; port < 20000; port++) {
        endpoint_t ep;
        ep.port = port;
        ep.addr.type = IP_TYPE_V4;
        ep.addr.v4 = IPV4_ADDR_UNSPECIFIED;

        // Check if there's a socket which is already binded to the port.
        if (tcp_lookup_local(&ep) != NULL) {
            continue;
        }

        memcpy(&sock->local, &ep, sizeof(sock->local));
        memcpy(&sock->remote.addr, dst_addr, sizeof(sock->remote.addr));
        sock->remote.port = dst_port;

        sock->state = TCP_STATE_SYN_SENT;
        tcp_set_pendings(sock, TCP_PEND_SYN);

        list_push_back(&active_socks, &sock->next);
        return;
    }

    // FIXME: return an error instead
    WARN("run out of client tcp ports");
}

void tcp_listen(tcp_sock_t sock, int backlog) {
    sock->state = TCP_STATE_LISTEN;
    sock->backlog = backlog;
    list_push_back(&active_socks, &sock->next);
}

tcp_sock_t tcp_accept(tcp_sock_t sock) {
    list_elem_t *elem = list_pop_front(&sock->backlog_socks);
    if (!elem) {
        return NULL;
    }

    struct tcp_socket *accepted_sock =
        LIST_CONTAINER(elem, struct tcp_socket, backlog_next);
    if (accepted_sock->state != TCP_STATE_ESTABLISHED) {
        // A 3-way handshake has not been finished.
        list_push_back(&sock->backlog_socks, &accepted_sock->backlog_next);
        return NULL;
    }

    return accepted_sock;
}

void tcp_write(tcp_sock_t sock, const void *data, size_t len) {
    mbuf_append_bytes(sock->tx_buf, data, len);
    sock->retransmit_at = 0;
}

size_t tcp_read(tcp_sock_t sock, void *buf, size_t buf_len) {
    size_t read_len = mbuf_read(&sock->rx_buf, buf, buf_len);
    sock->local_winsize += read_len;
    return read_len;
}

void tcp_transmit(tcp_sock_t sock) {
    if (sock->retransmit_at && sys_uptime() < sock->retransmit_at) {
        return;
    }

    uint32_t flags = tcp_clear_pendings(sock);
    mbuf_t payload = NULL;
    uint8_t ctrl_flags = 0;
    switch (sock->state) {
        case TCP_STATE_ESTABLISHED:
        case TCP_STATE_CLOSE_WAIT:
            payload = mbuf_peek(sock->tx_buf, sock->remote_winsize);
            if (mbuf_len(payload) > 0) {
                ctrl_flags |= TCP_ACK | TCP_PSH;
            }

            if (sock->last_seqno == sock->next_seqno) {
                sock->num_retransmits++;
            }
            break;
        case TCP_STATE_SYN_RECVED:
            ctrl_flags |= TCP_SYN | TCP_ACK;
            break;
        default:
            break;
    }

    if (flags & TCP_PEND_SYN) {
        ctrl_flags |= TCP_SYN;
    }

    if (flags & TCP_PEND_ACK) {
        ctrl_flags |= TCP_ACK;
    }

    if (flags & TCP_PEND_FIN) {
        ctrl_flags |= TCP_FIN;
    }

    if (!mbuf_len(payload) && !ctrl_flags) {
        // Nothing to send.
        mbuf_delete(payload);
        return;
    }

    // Construct a TCP packet.
    struct tcp_header header;
    header.src_port = hton16(sock->local.port);
    header.dst_port = hton16(sock->remote.port);
    header.seqno = hton32(sock->next_seqno);
    header.ackno = (ctrl_flags & TCP_ACK) ? hton32(sock->last_ack) : 0;
    header.off_and_ns = 5 << 4;
    header.flags = ctrl_flags;
    header.win_size = hton16(sock->local_winsize);
    header.checksum = 0;
    header.urgent = 0;

    // Look for the device to determine the source IP address to compute the
    // pseudo header checksum.
    struct device *device = device_lookup(&sock->remote.addr);
    if (!device) {
        // No route.
        WARN_DBG("no route");
        mbuf_delete(payload);
        return;
    }

    // Compute checksum.
    checksum_t checksum;
    checksum_init(&checksum);
    checksum_update_mbuf(&checksum, payload);
    checksum_update(&checksum, &header, sizeof(header));

    // Compute pseudo header checksum.
    switch (sock->remote.addr.type) {
        case IP_TYPE_V4: {
            size_t total_len = sizeof(header) + mbuf_len(payload);
            checksum_update_uint32(&checksum, hton32(sock->remote.addr.v4));
            checksum_update_uint32(&checksum, hton32(device->ipaddr.v4));
            checksum_update_uint16(&checksum, hton16(total_len));
            checksum_update_uint16(&checksum, hton16(IPV4_PROTO_TCP));
            break;
        }  // case IP_TYPE_V4
    }

    header.checksum = checksum_finish(&checksum);
    mbuf_t pkt = mbuf_new(&header, sizeof(header));
    if (payload) {
        mbuf_append(pkt, payload);
    }

    // Transmit the packet.
    switch (sock->remote.addr.type) {
        case IP_TYPE_V4:
            ipv4_transmit(sock->remote.addr.v4, IPV4_PROTO_TCP, pkt);
            break;
    }

    // Update the retransmission timer.
    sock->retransmit_at =
        sys_uptime()
        + MIN(TCP_RXT_MAX_TIMEOUT,
              TCP_RXT_INITIAL_TIMEOUT << MIN(sock->num_retransmits, 8));
    sock->last_seqno = sock->next_seqno;
}

static void tcp_process(struct tcp_socket *sock, ipaddr_t *src_addr,
                        port_t src_port, struct tcp_header *header,
                        mbuf_t payload) {
    uint32_t seq = ntoh32(header->seqno);
    uint32_t ack = ntoh32(header->ackno);
    uint8_t flags = header->flags;
    TRACE("tcp: port=%d, seq=%08x, ack=%08x, len=%d [ %s%s%s]",
          sock->local.port, seq, ack, mbuf_len(payload),
          (flags & TCP_SYN) ? "SYN " : "", (flags & TCP_FIN) ? "FIN " : "",
          (flags & TCP_ACK) ? "ACK " : "");

    // Handle a SYN packet destinated to a LISTENing socket.
    if (sock->state == TCP_STATE_LISTEN) {
        if ((flags & TCP_SYN) == 0) {
            stats.tcp_discarded++;
            return;
        }

        if (list_len(&sock->backlog_socks) >= sock->backlog) {
            stats.tcp_discarded++;
            return;
        }

        // Create a new socket for the client and reply SYN + ACK.
        TRACE("tcp: new client (port=%d)", sock->local.port);
        struct tcp_socket *new_sock = tcp_new();
        new_sock->state = TCP_STATE_SYN_RECVED;
        new_sock->last_ack = seq + 1;
        new_sock->listen_sock = sock;
        memcpy(&new_sock->local, &sock->local, sizeof(new_sock->local));
        memcpy(&new_sock->remote.addr, src_addr, sizeof(new_sock->remote.addr));
        new_sock->remote.port = src_port;
        list_push_back(&sock->backlog_socks, &new_sock->backlog_next);
        list_push_back(&active_socks, &new_sock->next);
        tcp_set_pendings(new_sock, TCP_PEND_ACK);
        return;
    }

    if (sock->state != TCP_STATE_SYN_SENT && seq != sock->last_ack) {
        // Unexpected sequence number.
        stats.tcp_discarded++;
        return;
    }

    sock->remote_winsize = ntoh32(header->win_size);
    if (sock->state == TCP_STATE_ESTABLISHED) {
        uint32_t acked_len = ack - sock->next_seqno;
        if (acked_len > 0) {
            mbuf_discard(&sock->tx_buf, acked_len);
            sock->next_seqno += acked_len;
        }
    }

    switch (sock->state) {
        case TCP_STATE_SYN_SENT: {
            if ((flags & (TCP_SYN | TCP_ACK)) == 0) {
                // Invalid (unexpected) packet, ignoring...
                stats.tcp_discarded++;
                break;
            }

            sock->next_seqno = ack;
            sock->last_ack = seq + 1;
            sock->state = TCP_STATE_ESTABLISHED;
            sock->retransmit_at = 0;
            break;
        }
        case TCP_STATE_SYN_RECVED: {
            if ((flags & TCP_ACK) == 0) {
                // Invalid (unexpected) packet, ignoring...
                stats.tcp_discarded++;
                break;
            }

            // Received an ACK to the our SYN + ACK. The connection is now
            // ESTABLISHED.
            sock->next_seqno = ack;
            sock->last_ack = seq;
            sock->state = TCP_STATE_ESTABLISHED;
            sock->retransmit_at = 0;

            struct event e;
            e.type = TCP_NEW_CLIENT;
            e.tcp_new_client.listen_sock = sock->listen_sock;
            sys_process_event(&e);
            break;
        }
        case TCP_STATE_ESTABLISHED:
            if (flags & TCP_FIN) {
                // Passive close. Acknowlege to FIN.
                tcp_set_pendings(sock, TCP_PEND_ACK);
                sock->state = TCP_STATE_CLOSE_WAIT;
                sock->retransmit_at = 0;
                sock->last_ack++;
            }

            // Received data. Copy into the receive buffer.
            TRACE("tcp: received %d bytes (seq=%x)", mbuf_len(payload), seq);
            size_t payload_len = mbuf_len(payload);
            if (payload_len > 0) {
                tcp_set_pendings(sock, TCP_PEND_ACK);

                if (sock->local_winsize < payload_len) {
                    // The receive buffer is full.
                    stats.tcp_dropped++;
                    break;
                }

                mbuf_append(sock->rx_buf, payload);
                sock->last_ack += mbuf_len(payload);
                sock->local_winsize -= payload_len;

                struct event e;
                e.type = TCP_RECEIVED;
                e.tcp_received.sock = sock;
                sys_process_event(&e);
            }
            break;
        case TCP_STATE_LAST_ACK:
            if ((flags & TCP_ACK) == 0) {
                // Invalid (unexpected) packet, ignoring...
                stats.tcp_discarded++;
                break;
            }

            TRACE("tcp: closed a socket (local=%d, remote=%d)",
                  sock->local.port, src_port);
            sock->state = TCP_STATE_CLOSED;
            break;
        default:
            break;
    }

    if (sock->state == TCP_STATE_CLOSE_WAIT && !mbuf_len(sock->tx_buf)) {
        // No pending TX data. Finish the connection.
        // FIXME: This transition should be initiated by the application.
        tcp_set_pendings(sock, TCP_PEND_FIN);
        sock->state = TCP_STATE_LAST_ACK;
    }
}

void tcp_receive(ipaddr_t *dst, ipaddr_t *src, mbuf_t pkt) {
    struct tcp_header header;
    if (mbuf_read(&pkt, &header, sizeof(header)) != sizeof(header)) {
        return;
    }

    size_t offset = (header.off_and_ns >> 4) * 4;
    if (offset > sizeof(header)) {
        mbuf_discard(&pkt, offset - sizeof(header));
    }

    uint16_t dst_port = ntoh16(header.dst_port);
    uint16_t src_port = ntoh16(header.src_port);

    endpoint_t dst_ep;
    dst_ep.port = dst_port;
    memcpy(&dst_ep.addr, dst, sizeof(ipaddr_t));
    endpoint_t src_ep;
    src_ep.port = src_port;
    memcpy(&src_ep.addr, src, sizeof(ipaddr_t));

    struct tcp_socket *sock = tcp_lookup(&dst_ep, &src_ep);
    if (!sock) {
        return;
    }

    tcp_process(sock, src, src_port, &header, pkt);
}

void tcp_flush(void) {
    LIST_FOR_EACH (sock, &active_socks, struct tcp_socket, next) {
        tcp_transmit(sock);
        LIST_FOR_EACH (backlog, &sock->backlog_socks, struct tcp_socket,
                       backlog_next) {
            tcp_transmit(backlog);
        }
    }
}

void tcp_init(void) {
    list_init(&active_socks);
    for (int i = 0; i < TCP_SOCKETS_MAX; i++) {
        sockets[i].in_use = false;
    }
}
