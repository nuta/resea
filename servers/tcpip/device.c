#include <resea/printf.h>
#include <string.h>
#include "device.h"
#include "dhcp.h"
#include "ipv4.h"

static struct device devices[DEVICES_MAX];

struct device *device_lookup(ipaddr_t *dst) {
    for (int i = 0; i < DEVICES_MAX; i++) {
        struct device *device = &devices[i];
        if (!device->in_use) {
            continue;
        }

        if (!ipaddr_equals_in_netmask(&device->ipaddr, dst, device->netmask)) {
            continue;
        }

        return device;
    }

    // Look for a device with the default gateway.
    for (int i = 0; i < DEVICES_MAX; i++) {
        struct device *device = &devices[i];
        if (!device->in_use) {
            continue;
        }

        if (ipaddr_is_unspecified(&device->gateway)) {
            continue;
        }

        return device;
    }

    return NULL;
}

bool device_dst_is_gateway(device_t device, ipaddr_t *dst) {
    return !ipaddr_equals_in_netmask(&device->ipaddr, dst, device->netmask);
}

device_t device_new(const char *name, transmit_fn_t transmit,
                    link_transmit_fn_t link_transmit, void *arg) {
    struct device *device = NULL;
    for (int i = 0; i < DEVICES_MAX; i++) {
        if (!devices[i].in_use) {
            device = &devices[i];
            break;
        }
    }

    if (!device) {
        return NULL;
    }

    device->in_use = true;
    device->arg = arg;
    device->transmit = transmit;
    device->link_transmit = link_transmit;
    device->dhcp_enabled = false;
    device->dhcp_leased = false;
    memset(device->macaddr, 0, MACADDR_LEN);
    memset(&device->ipaddr, 0, sizeof(ipaddr_t));
    memset(&device->netmask, 0, sizeof(ipaddr_t));
    memset(&device->gateway, 0, sizeof(ipaddr_t));
    strncpy(device->name, name, DEVICE_NAME_LEN);
    arp_init(&device->arp_table);
    return device;
}

void device_enable_dhcp(device_t device) {
    device->dhcp_enabled = true;
    device->dhcp_leased = false;
    dhcp_transmit(device, DHCP_TYPE_DISCOVER, IPV4_ADDR_UNSPECIFIED);
}

void device_set_macaddr(device_t device, macaddr_t *macaddr) {
    memcpy(device->macaddr, macaddr, MACADDR_LEN);
}

void device_set_ip_addrs(device_t device, ipaddr_t *ipaddr, ipaddr_t *netmask,
                         ipaddr_t *gateway) {
    memcpy(&device->ipaddr, ipaddr, sizeof(ipaddr_t));
    memcpy(&device->netmask, netmask, sizeof(ipaddr_t));
    memcpy(&device->gateway, gateway, sizeof(ipaddr_t));
}

void device_init(void) {
    for (int i = 0; i < DEVICES_MAX; i++) {
        devices[i].in_use = false;
    }
}
