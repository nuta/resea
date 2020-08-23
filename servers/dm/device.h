#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <resea/handle.h>
#include <types.h>
#include <list.h>

enum bus_type {
    BUS_TYPE_PCI,
};

struct device {
    list_elem_t next;
    handle_t handle;
    enum bus_type bus_type;
    union {
        struct pci_device pci;
    };
};

#endif
