#include <resea/ipc.h>
#include <resea/async.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <driver/irq.h>
#include <driver/io.h>
#include <driver/dma.h>
#include <virtio/virtio.h>
#include <endian.h>
#include <string.h>
#include "virtio_net.h"

static task_t tcpip_task;
static struct virtio_virtq *tx_virtq = NULL;
static struct virtio_virtq *rx_virtq = NULL;

static void read_macaddr(uint8_t *mac) {
    mac[0] = VIRTIO_DEVICE_CFG_READ8(struct virtio_net_config, mac0);
    mac[1] = VIRTIO_DEVICE_CFG_READ8(struct virtio_net_config, mac1);
    mac[2] = VIRTIO_DEVICE_CFG_READ8(struct virtio_net_config, mac2);
    mac[3] = VIRTIO_DEVICE_CFG_READ8(struct virtio_net_config, mac3);
    mac[4] = VIRTIO_DEVICE_CFG_READ8(struct virtio_net_config, mac4);
    mac[5] = VIRTIO_DEVICE_CFG_READ8(struct virtio_net_config, mac5);
}

static struct virtio_net_buffer *virtq_net_buffer(struct virtio_virtq *vq,
                                                  unsigned index) {
    return &((struct virtio_net_buffer *) vq->buffers)[index];
}

static void receive(const void *payload, size_t len);
void driver_handle_interrupt(void) {
    uint8_t status = virtio_read_isr_status();
    if (status & 1) {
        struct virtq_desc *desc;
        while ((desc = virtq_pop_desc(rx_virtq)) != NULL) {
            uint16_t id = from_le16(desc->id);
            uint32_t len = from_le32(desc->len);
            struct virtio_net_buffer *buf = virtq_net_buffer(rx_virtq, id);
            receive((const void *) buf->payload, len - sizeof(buf->header));
            buf->header.num_buffers = 1;
            virtq_push_desc(rx_virtq, desc);
        }

        virtq_notify(rx_virtq);
    }
}

static void receive(const void *payload, size_t len) {
    TRACE("received %d bytes", len);
    struct message m;
    m.type = NET_RX_MSG;
    m.net_rx.payload_len = len;
    m.net_rx.payload = (void *) payload;
    error_t err = ipc_send(tcpip_task, &m);
    ASSERT_OK(err);
}

static void transmit(void) {
    // Receive a packet to be sent.
    struct message m;
    ASSERT_OK(async_recv(tcpip_task, &m));
    ASSERT(m.type == NET_TX_MSG);

    // Allocate a desc for the transmission.
    size_t len = m.net_tx.payload_len;
    int index = virtq_alloc(tx_virtq, sizeof(struct virtio_net_header) + len);
    if (index < 0) {
        return;
    }

    // Fill the request.
    struct virtio_net_buffer *buf = virtq_net_buffer(tx_virtq, index);
    ASSERT(len <= sizeof(buf->payload));
    buf->header.flags = 0;
    buf->header.gso_type = VIRTIO_NET_HDR_GSO_NONE;
    buf->header.gso_size = 0;
    buf->header.checksum_start = 0;
    buf->header.checksum_offset = 0;
    buf->header.num_buffers = 0;
    memcpy((uint8_t *) &buf->payload, m.net_tx.payload, len);

    // Kick the device.
    virtq_notify(tx_virtq);
    free((void *) m.net_tx.payload);
}

void main(void) {
    TRACE("starting...");

    // Look for and initialize a virtio-net device.
    uint8_t irq;
    ASSERT_OK(virtio_pci_init(VIRTIO_DEVICE_NET, &irq));
    virtio_negotiate_feature(VIRTIO_NET_F_MAC | VIRTIO_NET_F_STATUS);
    virtio_init_virtqueues();

    // Allocate TX buffers.
    tx_virtq = virtq_get(VIRTIO_NET_QUEUE_TX);
    virtq_allocate_buffers(tx_virtq, sizeof(struct virtio_net_buffer), false);

    // Allocate RX buffers.
    rx_virtq = virtq_get(VIRTIO_NET_QUEUE_RX);
    virtq_allocate_buffers(rx_virtq, sizeof(struct virtio_net_buffer), true);
    for (int i = 0; i < rx_virtq->num_descs; i++) {
        struct virtio_net_buffer *buf = virtq_net_buffer(rx_virtq, i);
        buf->header.num_buffers = 1;
    }

    // Start listening for interrupts.
    ASSERT_OK(irq_acquire(irq));

    // Make the device active.
    virtio_activate();

    uint8_t mac[6];
    read_macaddr((uint8_t *) &mac);
    INFO("initialized the device");
    INFO("MAC address = %02x:%02x:%02x:%02x:%02x:%02x",
         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    ASSERT_OK(ipc_serve("net"));

    // Register this driver.
    tcpip_task = ipc_lookup("tcpip");
    ASSERT_OK(tcpip_task);
    struct message m;
    m.type = TCPIP_REGISTER_DEVICE_MSG;
    memcpy(m.tcpip_register_device.macaddr, mac, 6);
    ASSERT_OK(ipc_call(tcpip_task, &m));

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG:
                if (m.notifications.data & NOTIFY_IRQ) {
                    driver_handle_interrupt();
                }

                if (m.notifications.data & NOTIFY_ASYNC) {
                    transmit();
                }
                break;
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
