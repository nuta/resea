#ifndef __DHCP_H__
#define __DHCP_H__

#include <list.h>
#include "device.h"
#include "tcpip.h"

#define BOOTP_OP_REQUEST      1
#define BOOTP_HWTYPE_ETHERNET 1
#define DHCP_MAGIC            0x63825363

enum dhcp_option {
    DHCP_OPTION_NETMASK = 1,
    DHCP_OPTION_ROUTER = 3,
    DHCP_OPTION_REQUESTED_ADDR = 50,
    DHCP_OPTION_DHCP_TYPE = 53,
    DHCP_OPTION_PARAM_LIST = 55,
    DHCP_OPTION_END = 255,
};

enum dhcp_type {
    DHCP_TYPE_DISCOVER = 1,
    DHCP_TYPE_OFFER = 2,
    DHCP_TYPE_REQUEST = 3,
    DHCP_TYPE_ACK = 5,
};

struct dhcp_header {
    uint8_t op;
    uint8_t hw_type;
    uint8_t hw_len;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t client_ipaddr;
    uint32_t your_ipaddr;
    uint32_t server_ipaddr;
    uint32_t gateway_ipaddr;
    macaddr_t client_hwaddr;
    uint8_t unused[202];
    uint32_t magic;
    uint8_t options[];
} PACKED;

struct dhcp_type_option {
    uint8_t dhcp_type;
} PACKED;

struct dhcp_netmask_option {
    uint32_t netmask;
} PACKED;

struct dhcp_router_option {
    uint32_t router;
} PACKED;

#define DHCP_PARMAS_MAX 2
struct dhcp_params_option {
    uint8_t params[DHCP_PARMAS_MAX];
} PACKED;

struct dhcp_reqaddr_option {
    uint32_t ipaddr;
} PACKED;

void dhcp_transmit(device_t device, enum dhcp_type type,
                   ipv4addr_t requested_addr);
void dhcp_receive(void);
void dhcp_init(void);

#endif
