#include <resea.h>
#include <idl_stubs.h>
#include <server.h>
#include <driver/io.h>
#include <virtio.h>

// Channels connected by memmgr.
static cid_t memmgr_ch = 1;
static cid_t kernel_ch = 2;
// Connected by the discovery server.
static cid_t pci_server_ch;
// The channel at which we receive messages.
static cid_t server_ch;

static io_handle_t io_handle;
static struct virtio_device virtio;

static error_t handle_server_connect(struct message *m) {
    cid_t new_ch;
    TRY(open(&new_ch));
    transfer(new_ch, server_ch);

    m->header = SERVER_CONNECT_REPLY_MSG;
    m->payloads.server.connect_reply.ch = new_ch;
    return OK;
}

static error_t process_message(struct message *m) {
    switch (m->header) {
    case SERVER_CONNECT_MSG:  return handle_server_connect(m);
    }
    return ERR_UNEXPECTED_MESSAGE;
}

#define VIRTIO_BLK_T_IN  0
#define VIRTIO_BLK_T_OUT 1
struct virtio_blk_req {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
    uint8_t data[];
    // u8 status;
};

static error_t init_driver(void) {
    TRY(virtio_setup_pci_device(&virtio, memmgr_ch, pci_server_ch));
    TRY(virtio_setup_virtq(&virtio, 0));
    TRY(virtio_finish_init(&virtio));

    //
    // Try reading the first sector from the disk (incomplete)!
    //
    page_base_t page_base = valloc(1);
    int order = 0;
    uintptr_t buffer;
    size_t num_pages;
    paddr_t paddr;
    TRY(call_memory_alloc_phy_pages(memmgr_ch, 0, order,
                                    page_base, &buffer, &num_pages,
                                    &paddr));

    virtio.virtq->desc[0].paddr = paddr;
    virtio.virtq->desc[0].len   = sizeof(uint32_t) * 2 + sizeof(uint64_t);
    virtio.virtq->desc[0].flags = VIRTQ_DESC_F_NEXT;
    virtio.virtq->desc[0].next  = 1;

    virtio.virtq->desc[1].paddr = paddr + offsetof(struct virtio_blk_req, data);
    virtio.virtq->desc[1].len   = 512 + sizeof(uint8_t);
    virtio.virtq->desc[1].flags = VIRTQ_DESC_F_WRITE;
    virtio.virtq->desc[1].next  = 0;

    virtio.virtq->avail->ring[0] = 0;
    virtio.virtq->avail->index += 1;
    virtio.pci.notify_addrs[0] = 0;
    INFO("Sent a request to the device!");
    while (true) {
        volatile uint16_t *used_index = &virtio.virtq->used->index;
        if (*used_index > 0) {
            break;
        }
        INFO("Waiting for the device...");
    }

    INFO("Device proceeded the request");
    INFO("data = %ph", buffer + offsetof(struct virtio_blk_req, data), 16);

    return OK;
}

void main(void) {
    INFO("starting...");

    TRY_OR_PANIC(open(&server_ch));
    TRY_OR_PANIC(io_open(&io_handle, kernel_ch, IO_SPACE_IOPORT,
                         0, 0));

    TRY_OR_PANIC(call_discovery_connect(memmgr_ch, PCI_INTERFACE,
                                        &pci_server_ch));

    TRY_OR_PANIC(init_driver());

    TRY_OR_PANIC(server_register(memmgr_ch, server_ch,
                                 KEYBOARD_DRIVER_INTERFACE));

    INFO("entering the mainloop...");
    server_mainloop(server_ch, process_message);
}
