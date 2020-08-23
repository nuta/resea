#include <resea/ipc.h>
#include <resea/handle.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>
#include <list.h>
#include "pci.h"
#include "device.h"

static list_t devices;

void main(void) {
    TRACE("starting...");
    list_init(&devices);
    pci_init();

    ASSERT_OK(ipc_serve("dm"));

    INFO("ready");
    while (true) {
        struct message m;
        bzero(&m, sizeof(m));
        ASSERT_OK(ipc_recv(IPC_ANY, &m));

        switch (m.type) {
            case DM_ATTACH_PCI_DEVICE_MSG: {
                error_t err;
                int bus, slot;
                uint32_t vendor_id = m.dm_attach_pci_device.vendor_id;
                uint32_t device_id = m.dm_attach_pci_device.device_id;

                // Look for a PCI device...
                err = pci_find_device(vendor_id, device_id, &bus, &slot);
                if (err != OK) {
                    ipc_reply_err(m.src, err);
                    break;
                }

                // Found a matching device.
                struct device *dev = malloc(sizeof(*dev));
                dev->bus_type = BUS_TYPE_PCI;
                dev->handle = handle_alloc(m.src);
                pci_fill_pci_device_struct(&dev->pci, bus, slot);
                handle_set(m.src, dev->handle, dev);
                list_push_back(&devices, &dev->next);

                m.type = DM_ATTACH_PCI_DEVICE_REPLY_MSG;
                m.dm_attach_pci_device_reply.handle = dev->handle;
                m.dm_attach_pci_device_reply.vendor_id = dev->pci.vendor;
                m.dm_attach_pci_device_reply.device_id = dev->pci.device;
                ipc_reply(m.src, &m);
                break;
            }
            case DM_DETACH_DEVICE_MSG: {
                NYI();
            }
            case DM_PCI_GET_CONFIG_MSG: {
                struct device *dev = handle_get(m.src, m.dm_pci_get_config.handle);
                if (!dev) {
                    ipc_reply_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                m.type = DM_PCI_GET_CONFIG_REPLY_MSG;
                m.dm_pci_get_config_reply.bar0_addr = dev->pci.bar0_addr;
                m.dm_pci_get_config_reply.bar0_len = dev->pci.bar0_len;
                m.dm_pci_get_config_reply.irq = dev->pci.irq;
                ipc_reply(m.src, &m);
                break;
            }
            case DM_PCI_ENABLE_BUS_MASTER_MSG: {
                struct device *dev = handle_get(m.src, m.dm_pci_enable_bus_master.handle);
                if (!dev) {
                    ipc_reply_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                pci_enable_bus_master(&dev->pci);

                m.type = DM_PCI_ENABLE_BUS_MASTER_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
