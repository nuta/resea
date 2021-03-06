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
static dma_t tx_buffers_dma = NULL;
static dma_t rx_buffers_dma = NULL;
static struct virtio_ops *virtio = NULL;
static int tx_ring_index = 0;

static void read_macaddr(uint8_t *mac) {
    offset_t base = offsetof(struct virtio_net_config, mac);
    for (int i = 0; i < 6; i++) {
        mac[i] = virtio->read_device_config(base + i, sizeof(uint8_t));
    }
}

static struct virtio_net_buffer *get_buffer(dma_t dma, unsigned index,
                                            paddr_t *paddr) {
    offset_t offset = sizeof(struct virtio_net_buffer) * index;
    *paddr = dma_daddr(dma) + offset;
    return &((struct virtio_net_buffer *) dma_buf(dma))[index];
}

static struct virtio_net_buffer *get_buffer_by_paddr(dma_t dma, paddr_t paddr) {
    DEBUG_ASSERT(paddr >= dma_daddr(dma));

    offset_t offset = paddr - dma_daddr(dma);
    return (struct virtio_net_buffer *) (dma_buf(dma) + offset);
}

static void receive(const void *payload, size_t len);
void driver_handle_interrupt(void) {
    uint8_t status = virtio->read_isr_status();
    if (status & 1) {
        struct virtio_chain_entry chain[1];
        size_t total_len;
        while (true) {
            int n = virtio->virtq_pop(rx_virtq, chain, 1, &total_len);
            if (n == ERR_EMPTY) {
                break;
            }

            if (IS_ERROR(n)) {
                WARN_DBG("virtq_pop returned an error: %s", err2str(n));
                break;
            }

            if (n != 1) {
                WARN_DBG("virtq_pop returned unexpected # of descs: %d", n);
                break;
            }

            // Process the received packet.
            struct virtio_net_buffer *buf =
                get_buffer_by_paddr(rx_buffers_dma, chain[0].addr);
            receive((const void *) buf->payload,
                    total_len - sizeof(buf->header));

            // Enqueue the buffer back into the virtq.
            //
            // chain[0].addr is not modified by the device. We don't need to
            // update it.
            buf->header.num_buffers = 1;
            chain[0].len = sizeof(struct virtio_net_buffer);
            chain[0].device_writable = true;
            virtio->virtq_push(rx_virtq, chain, 1);
            virtio->virtq_notify(rx_virtq);
        }
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

    // Fill the request.
    int index = tx_ring_index++ % tx_virtq->num_descs;
    size_t len = m.net_tx.payload_len;
    paddr_t paddr;
    struct virtio_net_buffer *buf = get_buffer(tx_buffers_dma, index, &paddr);
    ASSERT(len <= sizeof(buf->payload));
    buf->header.flags = 0;
    buf->header.gso_type = VIRTIO_NET_HDR_GSO_NONE;
    buf->header.gso_size = 0;
    buf->header.checksum_start = 0;
    buf->header.checksum_offset = 0;
    buf->header.num_buffers = 0;
    memcpy((uint8_t *) &buf->payload, m.net_tx.payload, len);

    // Construct a descriptor chain for the reqeust.
    struct virtio_chain_entry chain[1];
    chain[0].addr = paddr;
    chain[0].len = sizeof(struct virtio_net_header) + len;
    chain[0].device_writable = false;

    // Kick the device.
    OOPS_OK(virtio->virtq_push(tx_virtq, chain, 1));
    virtio->virtq_notify(tx_virtq);

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
    rx_virtq = virtio->virtq_get(VIRTIO_NET_QUEUE_RX);
    tx_virtq = virtio->virtq_get(VIRTIO_NET_QUEUE_TX);

    // Allocate TX/RX buffers.
    tx_buffers_dma =
        dma_alloc(sizeof(struct virtio_net_buffer) * tx_virtq->num_descs,
                  DMA_ALLOC_FROM_DEVICE);
    rx_buffers_dma =
        dma_alloc(sizeof(struct virtio_net_buffer) * rx_virtq->num_descs,
                  DMA_ALLOC_FROM_DEVICE);

    // Fill the RX virtq.
    for (int i = 0; i < rx_virtq->num_descs; i++) {
        struct virtio_chain_entry chain[1];
        paddr_t paddr;
        struct virtio_net_buffer *buf = get_buffer(rx_buffers_dma, i, &paddr);
        buf->header.num_buffers = 1;
        chain[0].addr = paddr;
        chain[0].len = sizeof(struct virtio_net_buffer);
        chain[0].device_writable = true;
        virtio->virtq_push(rx_virtq, chain, 1);
    }
    virtio->virtq_notify(rx_virtq);

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
