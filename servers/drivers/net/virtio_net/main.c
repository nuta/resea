#include "virtio_net.h"
#include <driver/dma.h>
#include <driver/io.h>
#include <driver/irq.h>
#include <endian.h>
#include <resea/async.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>
#include <virtio/virtio.h>

static task_t tcpip_task;
static struct virtio_virtq *tx_virtq = NULL;
static struct virtio_virtq *rx_virtq = NULL;
static struct virtio_ops *virtio = NULL;

static void read_macaddr(uint8_t *mac) {
    offset_t base = offsetof(struct virtio_net_config, mac);
    for (int i = 0; i < 6; i++) {
        mac[i] = virtio->read_device_config(base + i, sizeof(uint8_t));
    }
}

static struct virtio_net_buffer *virtq_net_buffer(struct virtio_virtq *vq,
                                                  unsigned index) {
    return &((struct virtio_net_buffer *) vq->buffers)[index];
}

static void receive(const void *payload, size_t len);
void driver_handle_interrupt(void) {
    uint8_t status = virtio->read_isr_status();
    if (status & 1) {
        int index;
        size_t len;
        while (virtio->virtq_pop_desc(rx_virtq, &index, &len) == OK) {
            struct virtio_net_buffer *buf = virtq_net_buffer(rx_virtq, index);
            receive((const void *) buf->payload, len - sizeof(buf->header));
            buf->header.num_buffers = 1;
            virtio->virtq_push_desc(rx_virtq, index);
        }

        virtio->virtq_notify(rx_virtq);
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
    int index =
        virtio->virtq_alloc(tx_virtq, sizeof(struct virtio_net_header) + len, 0,
                            VIRTQ_ALLOC_NO_PREV);
    if (index < 0) {
        WARN("failed to alloc a desc for TX");
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
    virtio->virtq_kick_desc(tx_virtq, index);
    free(m.net_tx.payload);
}

void main(void) {
    TRACE("starting...");

    // Look for and initialize a virtio-net device.
    uint8_t irq;
    ASSERT_OK(virtio_find_device(VIRTIO_DEVICE_NET, &virtio, &irq));
    virtio->negotiate_feature(VIRTIO_NET_F_MAC | VIRTIO_NET_F_STATUS
                              | VIRTIO_NET_F_MRG_RXBUF);

    virtio->virtq_init(VIRTIO_NET_QUEUE_RX);
    virtio->virtq_init(VIRTIO_NET_QUEUE_TX);

    // Allocate TX buffers.
    tx_virtq = virtio->virtq_get(VIRTIO_NET_QUEUE_TX);
    virtio->virtq_allocate_buffers(tx_virtq, sizeof(struct virtio_net_buffer),
                                   false);

    // Allocate RX buffers.
    rx_virtq = virtio->virtq_get(VIRTIO_NET_QUEUE_RX);
    virtio->virtq_allocate_buffers(rx_virtq, sizeof(struct virtio_net_buffer),
                                   true);
    for (int i = 0; i < rx_virtq->num_descs; i++) {
        struct virtio_net_buffer *buf = virtq_net_buffer(rx_virtq, i);
        buf->header.num_buffers = 1;
    }

    // Start listening for interrupts.
    ASSERT_OK(irq_acquire(irq));

    // Make the device active.
    virtio->activate();

    uint8_t mac[6];
    read_macaddr((uint8_t *) &mac);
    INFO("initialized the device");
    INFO("MAC address = %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2],
         mac[3], mac[4], mac[5]);

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
