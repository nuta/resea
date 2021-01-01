#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "pci.h"
#include <list.h>
#include <resea/handle.h>
#include <types.h>

enum bus_type {
    BUS_TYPE_PCI,
};

struct device {
    list_elem_t next;
    task_t task;
    handle_t handle;
    enum bus_type bus_type;
    union {
        struct pci_device pci;
    };
};

#endif
