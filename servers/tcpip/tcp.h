#ifndef __TCP_H__
#define __TCP_H__

#include <list.h>
#include "mbuf.h"
#include "tcpip.h"

enum tcp_state {
    TCP_STATE_LISTEN,
    TCP_STATE_SYN_SENT,
    TCP_STATE_SYN_RECVED,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT1,
    TCP_STATE_FIN_WAIT2,
    TCP_STATE_CLOSING,
    TCP_STATE_TIME_WAIT,
    TCP_STATE_CLOSE_WAIT,
    TCP_STATE_LAST_ACK,
    TCP_STATE_CLOSED,
};

enum tcp_pending_flag {
    TCP_PEND_ACK = 1 << 0,
    TCP_PEND_FIN = 1 << 1,
};

struct client;

#define TCP_SOCKETS_MAX         512
#define TCP_RXT_INITIAL_TIMEOUT 500
#define TCP_RXT_MAX_TIMEOUT     5000
#define TCP_RX_BUF_SIZE         8192
struct tcp_socket {
    bool in_use;
    enum tcp_state state;
    uint32_t pendings;
    /// The first byte of unacknwoledged by the remote.
    uint32_t next_seqno;
    /// The seqno which we have sent.
    uint32_t last_seqno;
    /// The last byte received from the remote.
    uint32_t last_ack;
    uint32_t local_winsize;
    uint32_t remote_winsize;
    endpoint_t local;
    endpoint_t remote;
    mbuf_t rx_buf;
    mbuf_t tx_buf;
    size_t backlog;
    unsigned num_retransmits;
    msec_t retransmit_at;
    struct tcp_socket *listen_sock;
    list_t backlog_socks;
    list_elem_t next;
    list_elem_t backlog_next;
    struct client *client;
};

typedef struct tcp_socket *tcp_sock_t;

enum tcp_header_flag {
    TCP_FIN = 1 << 0,
    TCP_SYN = 1 << 1,
    TCP_RST = 1 << 2,
    TCP_PSH = 1 << 3,
    TCP_ACK = 1 << 4,
};

struct tcp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seqno;
    uint32_t ackno;
    uint8_t off_and_ns;
    uint8_t flags;
    uint16_t win_size;
    uint16_t checksum;
    uint16_t urgent;
} __packed;

tcp_sock_t tcp_new(void);
void tcp_close(tcp_sock_t sock);
void tcp_bind(tcp_sock_t sock, ipaddr_t *addr, port_t port);
void tcp_listen(tcp_sock_t sock, int backlog);
tcp_sock_t tcp_accept(tcp_sock_t sock);
void tcp_write(tcp_sock_t sock, const void *data, size_t len);
size_t tcp_read(tcp_sock_t sock, void *buf, size_t buf_len);
void tcp_transmit(tcp_sock_t sock);
void tcp_receive(ipaddr_t *dst, ipaddr_t *src, mbuf_t pkt);
void tcp_flush(void);
void tcp_init(void);

#endif
