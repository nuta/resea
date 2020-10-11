#include <endian.h>
#include <list.h>
#include <string.h>
#include <resea/malloc.h>
#include "device.h"
#include "dns.h"
#include "tcpip.h"
#include "udp.h"
#include "sys.h"

static udp_sock_t udp_sock;
static ipaddr_t dns_server_ipaddr;

error_t dns_query(uint16_t id, const char *hostname) {
    struct dns_header header;
    header.id = hton16(id);
    header.flags = hton16(0x0100); // a standard query (recursive)
    header.num_queries = hton16(1);
    header.num_answers = 0;
    header.num_authority = 0;
    header.num_additional = 0;

    char *s = strdup(hostname);
    mbuf_t m = mbuf_new(&header, sizeof(header));
    while (*s != '\0') {
        char *label = s;
        while (*s != '.' && *s != '\0') {
            s++;
        }

        *s = '\0';
        size_t label_len = strlen(label);
        if (label_len > 63) {
            WARN_DBG("dns: too long hostname: %s", hostname);
            return ERR_INVALID_ARG;
        }

        mbuf_append_bytes(m, &(uint8_t){ label_len }, 1);
        mbuf_append_bytes(m, label, strlen(label));
        s++;
    }

    uint16_t qtype_ne = hton16(DNS_QTYPE_A);
    uint16_t qclass_ne = hton16(0x0001);
    mbuf_append_bytes(m,  &(uint8_t){ 0 }, 1);
    mbuf_append_bytes(m, (uint8_t *) &qtype_ne, sizeof(uint16_t));
    mbuf_append_bytes(m, (uint8_t *) &qclass_ne, sizeof(uint16_t));

    udp_sendto_mbuf(udp_sock, &dns_server_ipaddr, 53, m);
    udp_transmit(udp_sock);
    return OK;
}

static void skip_labels(mbuf_t *payload) {
    uint8_t len;
    do {
        if (mbuf_read(payload, &len, sizeof(len)) != sizeof(len)) {
            return;
        }

        // Handle 4.1.4. Message compression in RFC1035.
        if (len & 0xc0) {
            mbuf_discard(payload, 1);
            break;
        }

        mbuf_discard(payload, len);
    } while (len > 0);
}

static void dns_process(struct device *device, mbuf_t payload) {
    struct dns_header header;
    if (mbuf_read(&payload, &header, sizeof(header)) != sizeof(header)) {
        return;
    }

    // Skip queries.
    uint16_t num_queries = ntoh16(header.num_queries);
    for (uint16_t i = 0; i < num_queries; i++) {
        skip_labels(&payload);
        struct dns_query_footer footer;
        if (mbuf_read(&payload, &footer, sizeof(footer)) != sizeof(footer)) {
            return;
        }
    }

    uint16_t num_answers = ntoh16(header.num_answers);
    for (uint16_t i = 0; i < num_answers; i++) {
        skip_labels(&payload);
        struct dns_answer_footer footer;
        if (mbuf_read(&payload, &footer, sizeof(footer)) != sizeof(footer)) {
            return;
        }

        uint16_t data_len = ntoh16(footer.len);
        if (ntoh16(footer.qtype) != DNS_QTYPE_A) {
            mbuf_discard(&payload, data_len);
            continue;
        }

        uint32_t data;
        if (mbuf_read(&payload, &data, sizeof(data)) != sizeof(data)) {
            return;
        }

        uint32_t id = ntoh16(header.id);
        ipv4addr_t addr = ntoh32(data);

        struct event e;
        e.type = DNS_GOT_ANSWER;
        e.dns_got_answer.id = id;
        e.dns_got_answer.addr = addr;
        sys_process_event(&e);
    }
}

void dns_receive(void) {
    while (true) {
        device_t device;
        ipaddr_t src;
        port_t src_port;
        mbuf_t payload = udp_recv_mbuf(udp_sock, &device, &src, &src_port);
        if (!payload) {
            break;
        }

        if (!ipaddr_equals(&src, &dns_server_ipaddr)) {
            WARN_DBG("received a DNS answer from an unknown address %pI4", src);
            continue;
        }

        dns_process(device, payload);
    }
}

void dns_set_name_server(ipaddr_t *ipaddr) {
    memcpy(&dns_server_ipaddr, ipaddr, sizeof(dns_server_ipaddr));
}

void dns_init(void) {
    udp_sock = udp_new();
    udp_bind(udp_sock,
             &(ipaddr_t){.type = IP_TYPE_V4, .v4 = IPV4_ADDR_UNSPECIFIED}, 3535);
}
