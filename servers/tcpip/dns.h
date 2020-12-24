#ifndef __DNS_H__
#define __DNS_H__

#include "tcpip.h"
#include <types.h>

#define DNS_QTYPE_A 0x0001

struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t num_queries;
    uint16_t num_answers;
    uint16_t num_authority;
    uint16_t num_additional;
} __packed;

struct dns_query_footer {
    uint16_t qtype;
    uint16_t qclass;
} __packed;

struct dns_answer_footer {
    uint16_t qtype;
    uint16_t qclass;
    uint32_t ttl;
    uint16_t len;
    uint8_t data[];
} __packed;

error_t dns_query(uint16_t id, const char *hostname);
void dns_receive(void);
void dns_set_name_server(ipaddr_t *ipaddr);
void dns_init(void);

#endif
