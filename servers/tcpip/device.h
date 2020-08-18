#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "arp.h"
#include "mbuf.h"
#include "tcpip.h"

#define DEVICES_MAX     8
#define DEVICE_NAME_LEN 8
struct device;

typedef void (*transmit_fn_t)(struct device *device, enum ether_type type,
                              ipaddr_t *dst, mbuf_t payload);
/// This function MUST delete the payload mbuf.
typedef void (*link_transmit_fn_t)(struct device *device, mbuf_t pkt);

struct device {
    bool in_use;
    char name[DEVICE_NAME_LEN];
    void *arg;
    bool dhcp_enabled;
    bool dhcp_leased;
    macaddr_t macaddr;
    ipaddr_t ipaddr;
    ipaddr_t gateway;
    ipaddr_t netmask;
    struct arp_table arp_table;
    transmit_fn_t transmit;
    link_transmit_fn_t link_transmit;
};

typedef struct device *device_t;

struct device *device_lookup(ipaddr_t *dst);
device_t device_new(const char *name, transmit_fn_t transmit,
                    link_transmit_fn_t link_transmit, void *arg);
void device_enable_dhcp(device_t device);
bool device_dst_is_gateway(device_t device, ipaddr_t *dst);
void device_set_macaddr(device_t device, macaddr_t *macaddr);
void device_set_ip_addrs(device_t device, ipaddr_t *ipaddr, ipaddr_t *netmask,
                         ipaddr_t *gateway);
void device_init(void);

#endif
