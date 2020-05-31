#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/io.h>
#include <cstring.h>
#include "e1000.h"
#include "pci.h"

static task_t tcpip_tid;

static void receive(const void *payload, size_t len) {
    TRACE("received %d bytes", len);
    struct message m;
    m.type = NET_RX_MSG;
    m.net_rx.payload_len = len;
    m.net_rx.payload = (void *) payload;
    error_t err = ipc_send(tcpip_tid, &m);
    ASSERT_OK(err);
}

void transmit(void) {
    struct message m;
    m.type = NET_GET_TX_MSG;
    ASSERT_OK(ipc_call(tcpip_tid, &m));
    ASSERT(m.type == NET_TX_MSG);
    e1000_transmit(m.net_tx.payload, m.net_tx.payload_len);
    free((void *) m.net_tx.payload);
}

void main(void) {
    error_t err;
    TRACE("starting...");

    struct pci_device pcidev;
    if (!pci_find_device(&pcidev, 0x8086, 0x100e)) {
        PANIC("failed to locate a e1000 device");
    }

    INFO("found a e1000 device (bus=%d, slot=%d, bar0=%x, irq=%d)", pcidev.bus,
         pcidev.slot, pcidev.bar0, pcidev.irq);

    // Initialize the device and listen for IRQ messages.
    err = irq_acquire(pcidev.irq);
    ASSERT_OK(err);
    pci_enable_bus_master(&pcidev);
    e1000_init(&pcidev);

    uint8_t mac[6];
    e1000_read_macaddr((uint8_t *) &mac);
    INFO("initialized the device");
    INFO("MAC address = %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2],
         mac[3], mac[4], mac[5]);

    // Wait for the tcpip server.
    tcpip_tid = ipc_lookup("tcpip");
    ASSERT_OK(tcpip_tid);

    // Register this driver.
    struct message m;
    m.type = TCPIP_REGISTER_DEVICE_MSG;
    memcpy(m.tcpip_register_device.macaddr, mac, 6);
    err = ipc_call(tcpip_tid, &m);
    ASSERT_OK(err);

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG:
                if (m.notifications.data & NOTIFY_IRQ) {
                    e1000_handle_interrupt(receive);
                }

                if (m.notifications.data & NOTIFY_NEW_DATA) {
                    transmit();
                }
                break;
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
