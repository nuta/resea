#include <list.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include "dhcp.h"
#include <endian.h>
#include "udp.h"

static udp_sock_t udp_sock;

void dhcp_transmit(device_t device, enum dhcp_type type,
                   ipv4addr_t requested_addr) {
    struct dhcp_header header;
    header.op = BOOTP_OP_REQUEST;
    header.hw_type = BOOTP_HWTYPE_ETHERNET;
    header.hw_len = MACADDR_LEN;
    header.hops = 0;
    header.xid = hton32(0x1234abcd);
    header.secs = 0;
    header.flags = 0;
    header.client_ipaddr = 0;
    header.your_ipaddr = 0;
    header.server_ipaddr = 0;
    header.gateway_ipaddr = 0;
    memcpy(header.client_hwaddr, device->macaddr, MACADDR_LEN);
    header.magic = hton32(DHCP_MAGIC);
    memset(&header.unused, 0, sizeof(header.unused));

    mbuf_t m = mbuf_new(&header, sizeof(header));

    // DHCP Type Option
    struct dhcp_type_option type_opt;
    type_opt.dhcp_type = type;
    mbuf_append_bytes(m, &(uint8_t[2]){DHCP_OPTION_DHCP_TYPE, sizeof(type_opt)},
                      2);
    mbuf_append_bytes(m, &type_opt, sizeof(type_opt));

    // DHCP Requested IP address Option
    if (requested_addr != IPV4_ADDR_UNSPECIFIED) {
        struct dhcp_reqaddr_option addr_opt;
        addr_opt.ipaddr = hton32(requested_addr);
        mbuf_append_bytes(
            m, &(uint8_t[2]){DHCP_OPTION_REQUESTED_ADDR, sizeof(addr_opt)}, 2);
        mbuf_append_bytes(m, &addr_opt, sizeof(addr_opt));
    }

    // DHCP Parameter Request List Option
    struct dhcp_params_option params_opt;
    params_opt.params[0] = DHCP_OPTION_NETMASK;
    params_opt.params[1] = DHCP_OPTION_ROUTER;
    mbuf_append_bytes(
        m, &(uint8_t[2]){DHCP_OPTION_PARAM_LIST, sizeof(params_opt)}, 2);
    mbuf_append_bytes(m, &params_opt, sizeof(params_opt));

    // DHCP End Option
    mbuf_append_bytes(m, &(uint8_t){DHCP_OPTION_END}, 1);

    // Padding
    size_t padding_len = 4 - (mbuf_len(m) % 4) + 8;
    while (padding_len-- > 0) {
        mbuf_append_bytes(m, &(uint8_t){DHCP_OPTION_END}, 1);
    }

    udp_sendto_mbuf(udp_sock,
                    &(ipaddr_t){.type = IP_TYPE_V4, .v4 = IPV4_ADDR_BROADCAST},
                    67, m);
    udp_transmit(udp_sock);
}

static void dhcp_process(struct device *device, mbuf_t payload) {
    struct dhcp_header header;
    if (mbuf_read(&payload, &header, sizeof(header)) != sizeof(header)) {
        return;
    }

    ipv4addr_t your_ipaddr = ntoh32(header.your_ipaddr);

    // Parse DHCP options.
    enum dhcp_type type = 0;
    ipv4addr_t gateway = IPV4_ADDR_UNSPECIFIED;
    ipv4addr_t netmask = IPV4_ADDR_UNSPECIFIED;
    while (mbuf_is_empty(payload)) {
        uint8_t option_type;
        if (mbuf_read(&payload, &option_type, 1) != 1) {
            break;
        }

        if (option_type == DHCP_OPTION_END) {
            break;
        }

        uint8_t option_len;
        if (mbuf_read(&payload, &option_len, 1) != 1) {
            break;
        }

        switch (option_type) {
            case DHCP_OPTION_DHCP_TYPE: {
                struct dhcp_type_option opt;
                if (mbuf_read(&payload, &opt, sizeof(opt)) != sizeof(opt)) {
                    break;
                }

                type = opt.dhcp_type;
                break;
            }
            case DHCP_OPTION_NETMASK: {
                struct dhcp_netmask_option opt;
                if (mbuf_read(&payload, &opt, sizeof(opt)) != sizeof(opt)) {
                    break;
                }

                netmask = ntoh32(opt.netmask);
                break;
            }
            case DHCP_OPTION_ROUTER: {
                struct dhcp_router_option opt;
                if (mbuf_read(&payload, &opt, sizeof(opt)) != sizeof(opt)) {
                    break;
                }

                gateway = ntoh32(opt.router);
                break;
            }
        }
    }

    switch (type) {
        case DHCP_TYPE_OFFER:
            dhcp_transmit(device, DHCP_TYPE_REQUEST, your_ipaddr);
            break;
        case DHCP_TYPE_ACK:
            if (netmask == IPV4_ADDR_UNSPECIFIED
                || gateway == IPV4_ADDR_UNSPECIFIED) {
                WARN(
                    "netmask or router DHCP option is not included, "
                    "discarding a DHCP ACK..");
                break;
            }

            INFO("dhcp: leased ip=%pI4, netmask=%pI4, gateway=%pI4",
                 your_ipaddr, netmask, gateway);
            device->dhcp_leased = true;
            device_set_ip_addrs(
                device, &(ipaddr_t){.type = IP_TYPE_V4, .v4 = your_ipaddr},
                &(ipaddr_t){.type = IP_TYPE_V4, .v4 = netmask},
                &(ipaddr_t){.type = IP_TYPE_V4, .v4 = gateway});
            break;
        default:
            WARN("ignoring a DHCP message (dhcp_type=%d)", type);
    }
}

void dhcp_receive(void) {
    while (true) {
        device_t device;
        ipaddr_t src;
        port_t src_port;
        mbuf_t payload = udp_recv_mbuf(udp_sock, &device, &src, &src_port);
        if (!payload) {
            break;
        }

        dhcp_process(device, payload);
    }
}

void dhcp_init(void) {
    udp_sock = udp_new();
    udp_bind(udp_sock,
             &(ipaddr_t){.type = IP_TYPE_V4, .v4 = IPV4_ADDR_UNSPECIFIED}, 68);
}
