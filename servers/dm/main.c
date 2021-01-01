#include "device.h"
#include "pci.h"
#include <list.h>
#include <resea/async.h>
#include <resea/handle.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>

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
            case NOTIFICATIONS_MSG: {
                if (m.notifications.data & NOTIFY_ASYNC) {
                    async_recv(VM_TASK, &m);
                    switch (m.type) {
                        case TASK_EXITED_MSG: {
                            LIST_FOR_EACH (dev, &devices, struct device, next) {
                                if (dev->task == m.task_exited.task) {
                                    list_remove(&dev->next);
                                    free(dev);
                                }
                            }
                            break;
                        }
                        default:
                            discard_unknown_message(&m);
                    }
                }

                break;
            }
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
                dev->task = m.src;
                pci_fill_pci_device_struct(&dev->pci, bus, slot);
                handle_set(m.src, dev->handle, dev);
                list_push_back(&devices, &dev->next);

                m.type = TASK_WATCH_MSG;
                m.task_watch.task = dev->task;
                ASSERT_OK(ipc_call(VM_TASK, &m));

                m.type = DM_ATTACH_PCI_DEVICE_REPLY_MSG;
                m.dm_attach_pci_device_reply.handle = dev->handle;
                m.dm_attach_pci_device_reply.vendor_id = dev->pci.vendor;
                m.dm_attach_pci_device_reply.device_id = dev->pci.device;
                ipc_reply(dev->task, &m);
                break;
            }
            case DM_DETACH_DEVICE_MSG: {
                NYI();
            }
            case DM_PCI_GET_CONFIG_MSG: {
                struct device *dev =
                    handle_get(m.src, m.dm_pci_get_config.handle);
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
            case DM_PCI_READ_CONFIG_MSG: {
                struct device *dev =
                    handle_get(m.src, m.dm_pci_read_config.handle);
                if (!dev || dev->bus_type != BUS_TYPE_PCI) {
                    ipc_reply_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                uint32_t value =
                    pci_read_config(&dev->pci, m.dm_pci_read_config.offset,
                                    m.dm_pci_read_config.size);

                m.type = DM_PCI_READ_CONFIG_REPLY_MSG;
                m.dm_pci_read_config_reply.value = value;
                ipc_reply(m.src, &m);
                break;
            }
            case DM_PCI_WRITE_CONFIG_MSG: {
                struct device *dev =
                    handle_get(m.src, m.dm_pci_write_config.handle);
                if (!dev || dev->bus_type != BUS_TYPE_PCI) {
                    ipc_reply_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                pci_write_config(&dev->pci, m.dm_pci_write_config.offset,
                                 m.dm_pci_write_config.size,
                                 m.dm_pci_write_config.value);

                m.type = DM_PCI_WRITE_CONFIG_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            case DM_PCI_ENABLE_BUS_MASTER_MSG: {
                struct device *dev =
                    handle_get(m.src, m.dm_pci_enable_bus_master.handle);
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
                discard_unknown_message(&m);
        }
    }
}
