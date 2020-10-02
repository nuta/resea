#ifndef __ICMP_H__
#define __ICMP_H__

#include <list.h>
#include "ethernet.h"
#include "mbuf.h"
#include "tcpip.h"

#define ICMP_TYPE_ECHO_REPLY    0x00
#define ICMP_TYPE_ECHO_REQUEST  0x08

struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq_no;
} __packed;

void icmp_send_echo_request(ipaddr_t *dst);
void icmp_receive(ipaddr_t *dst, ipaddr_t *src, mbuf_t pkt);

#endif
