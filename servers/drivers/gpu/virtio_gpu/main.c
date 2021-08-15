#include "virtio_gpu.h"
#include <driver/dma.h>
#include <driver/io.h>
#include <driver/irq.h>
#include <endian.h>
#include <ipc_client/shm.h>
#include <resea/async.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/timer.h>
#include <string.h>
#include <virtio/virtio.h>

static struct virtio_virtq *ctrlq = NULL;
static struct virtio_ops *virtio = NULL;
static uint32_t display_width = 0;
static uint32_t display_height = 0;
static int next_resource_id = 1;
static int display_resource_id = -1;
static dma_t req_buffers_dma = NULL;
static dma_t resp_buffers_dma = NULL;
static int next_buffer_index = 0;
static uint32_t *framebuffer = NULL;
enum driver_state driver_state = LOOKING_FOR_DISPLAY;

static handle_t shm_handle;
static void *shm_framebuffer = NULL;
size_t framebuffer_size;

static void execute_command(struct virtio_gpu_ctrl_hdr *cmd, size_t len) {
    ASSERT(len <= VIRTIO_GPU_CTRLQ_ENTRY_MAX_SIZE);

    int index = next_buffer_index++ % ctrlq->num_descs;
    offset_t off = index * VIRTIO_GPU_CTRLQ_ENTRY_MAX_SIZE;

    memcpy(dma_buf(req_buffers_dma) + off, cmd, len);

    struct virtio_chain_entry chain[2];
    chain[0].addr = dma_daddr(req_buffers_dma) + off;
    chain[0].len = len;
    chain[0].device_writable = false;
    chain[1].addr = dma_daddr(resp_buffers_dma) + off;
    chain[1].len = VIRTIO_GPU_CTRLQ_ENTRY_MAX_SIZE;
    chain[1].device_writable = true;

    virtio->virtq_push(ctrlq, chain, 2);
    virtio->virtq_notify(ctrlq);
}

static void initialize_display(uint32_t width, uint32_t height) {
    if (driver_state != LOOKING_FOR_DISPLAY) {
        return;
    }

    display_width = width;
    display_height = height;
    display_resource_id = next_resource_id++;

    struct virtio_gpu_resource_create_2d cmd;
    cmd.hdr.type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D;
    cmd.hdr.flags = 0;
    cmd.hdr.fence_id = 0;
    cmd.hdr.ctx_id = 0;
    cmd.hdr.padding = 0;
    cmd.resource_id = display_resource_id;
    cmd.format = VIRTIO_GPU_FORMAT_R8G8B8X8_UNORM;
    cmd.height = display_height;
    cmd.width = display_width;
    execute_command((struct virtio_gpu_ctrl_hdr *) &cmd, sizeof(cmd));
    driver_state = CREATING_2D_RESOURCE;
}

static void initialize_framebuffer(void) {
    framebuffer_size = display_height * display_width * sizeof(uint32_t);
    dma_t framebuffer_dma = dma_alloc(framebuffer_size, DMA_ALLOC_TO_DEVICE);
    framebuffer = (uint32_t *) dma_buf(framebuffer_dma);
    memset(framebuffer, 0 /* fill with black */, framebuffer_size);

    ASSERT_OK(shm_create(framebuffer_size, &shm_handle, &shm_framebuffer));

    TRACE("attaching the framebuffer (%d KiB)", framebuffer_size / 1024);
    struct virtio_gpu_resource_attach_backing cmd;
    cmd.hdr.type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING;
    cmd.hdr.flags = 0;
    cmd.hdr.fence_id = 0;
    cmd.hdr.ctx_id = 0;
    cmd.hdr.padding = 0;
    cmd.resource_id = display_resource_id;
    cmd.nr_entries = 1;
    cmd.entries[0].addr = dma_daddr(framebuffer_dma);
    cmd.entries[0].length = framebuffer_size;
    cmd.entries[0].padding = 0;
    execute_command((struct virtio_gpu_ctrl_hdr *) &cmd, sizeof(cmd));
    driver_state = ATTACHING_FRAMEBUFFER;
}

static void initialize_scanout(void) {
    TRACE("setting the scanout");
    struct virtio_gpu_set_scanout cmd;
    cmd.hdr.type = VIRTIO_GPU_CMD_SET_SCANOUT;
    cmd.hdr.flags = 0;
    cmd.hdr.fence_id = 0;
    cmd.hdr.ctx_id = 0;
    cmd.hdr.padding = 0;
    cmd.resource_id = display_resource_id;
    cmd.scanout_id = 0;
    cmd.r.height = display_height;
    cmd.r.width = display_width;
    cmd.r.x = 0;
    cmd.r.y = 0;
    execute_command((struct virtio_gpu_ctrl_hdr *) &cmd, sizeof(cmd));
    driver_state = SETTING_SCANOUT;
}

static void start_flushing_scanout(void) {
    TRACE("transfering the framebuffer");
    struct virtio_gpu_transfer_to_host_2d cmd;
    cmd.hdr.type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
    cmd.hdr.flags = 0;
    cmd.hdr.fence_id = 0;
    cmd.hdr.ctx_id = 0;
    cmd.hdr.padding = 0;
    cmd.offset = 0;
    cmd.resource_id = display_resource_id;
    cmd.padding = 0;
    cmd.r.height = display_height;
    cmd.r.width = display_width;
    cmd.r.x = 0;
    cmd.r.y = 0;
    execute_command((struct virtio_gpu_ctrl_hdr *) &cmd, sizeof(cmd));
    driver_state = TRANSFERING_TO_HOST;
}

static void flush_scanout(void) {
    TRACE("flushing the scanout");
    struct virtio_gpu_resource_flush cmd;
    cmd.hdr.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
    cmd.hdr.flags = 0;
    cmd.hdr.fence_id = 0;
    cmd.hdr.ctx_id = 0;
    cmd.hdr.padding = 0;
    cmd.resource_id = display_resource_id;
    cmd.padding = 0;
    cmd.r.height = display_height;
    cmd.r.width = display_width;
    cmd.r.x = 0;
    cmd.r.y = 0;
    TRACE("resource_flush=%d", sizeof(cmd));
    execute_command((struct virtio_gpu_ctrl_hdr *) &cmd, sizeof(cmd));
    driver_state = FLUSHING_SCANOUT;
}

static void handle_ok_nodata(void) {
    switch (driver_state) {
        case CREATING_2D_RESOURCE:
            initialize_framebuffer();
            break;
        case ATTACHING_FRAMEBUFFER:
            initialize_scanout();
            break;
        case SETTING_SCANOUT:
            INFO("successfully enabled the display!");
            driver_state = DISPLAY_ENABLED;
            start_flushing_scanout();
            break;
        case TRANSFERING_TO_HOST:
            flush_scanout();
            break;
        case FLUSHING_SCANOUT:
            driver_state = DISPLAY_ENABLED;
            break;
        default:
            WARN_DBG("unexpected VIRTIO_GPU_RESP_OK_NODATA");
    }
}

static void handle_display_info(struct virtio_gpu_resp_display_info *r) {
    for (int i = 0; i < VIRTIO_GPU_MAX_SCANOUTS; i++) {
        struct virtio_gpu_display_one *pmode = &r->pmodes[i];
        if (pmode->enabled) {
            TRACE("found a display: resolution=%dx%d", pmode->r.width,
                  pmode->r.height);
            initialize_display(pmode->r.width, pmode->r.height);
            break;
        }
    }
}

static void handle_interrupt(void) {
    uint8_t status = virtio->read_isr_status();
    if (status & 1) {
        struct virtio_chain_entry chain[2];
        size_t total_len;
        while (true) {
            int n = virtio->virtq_pop(ctrlq, chain, 2, &total_len);
            if (n == ERR_EMPTY) {
                break;
            }

            ASSERT(n == 2);
            offset_t off = chain[1].addr - dma_daddr(resp_buffers_dma);
            struct virtio_gpu_ctrl_hdr *hdr =
                (struct virtio_gpu_ctrl_hdr *) (dma_buf(resp_buffers_dma)
                                                + off);
            switch (hdr->type) {
                case VIRTIO_GPU_RESP_OK_NODATA:
                    handle_ok_nodata();
                    break;
                case VIRTIO_GPU_RESP_OK_DISPLAY_INFO:
                    handle_display_info(
                        (struct virtio_gpu_resp_display_info *) hdr);
                    break;
                default:
                    WARN_DBG("unknown response from the device: type=0x%x",
                             hdr->type);
            }
        }
    }
}

static void switch_front_buffer(int index) {
    // TODO: Switch the buffer by sending a command.
    memcpy(framebuffer, shm_framebuffer, framebuffer_size);
}

/// Looks for and initializes a virtio-gpu device.
static void init(void) {
    uint8_t irq;
    ASSERT_OK(virtio_find_device(VIRTIO_DEVICE_GPU, &virtio, &irq));
    virtio->negotiate_feature(0 /* No essential features for virito-gpu. */);

    virtio->virtq_init(VIRTIO_GPU_QUEUE_CTRL);
    virtio->virtq_init(VIRTIO_GPU_QUEUE_CURSOR);
    ctrlq = virtio->virtq_get(VIRTIO_GPU_QUEUE_CTRL);
    virtio->activate();

    req_buffers_dma =
        dma_alloc(VIRTIO_GPU_CTRLQ_ENTRY_MAX_SIZE * ctrlq->num_descs,
                  DMA_ALLOC_FROM_DEVICE);
    resp_buffers_dma =
        dma_alloc(VIRTIO_GPU_CTRLQ_ENTRY_MAX_SIZE * ctrlq->num_descs,
                  DMA_ALLOC_FROM_DEVICE);

    ASSERT_OK(irq_acquire(irq));

    // We've finished virtio device initialization. For virito-gpu, we need some
    // extra initialization steps...

    struct virtio_gpu_ctrl_hdr hdr;
    hdr.type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO;
    hdr.flags = 0;
    hdr.fence_id = 0;
    hdr.ctx_id = 0;
    hdr.padding = 0;
    execute_command(&hdr, sizeof(hdr));
}

#ifdef CONFIG_VIRTIO_GPU_DEMO
static uint32_t pick_color(int y) {
    uint32_t colors[] = {
        0xff0000, 0x800000, 0xffff00, 0x808000, 0x00ff00, 0x008000,
        0x00ffff, 0x008080, 0x0000ff, 0x000080, 0xff00ff, 0x800080,
    };

    return colors[y % (sizeof(colors) / sizeof(uint32_t))];
}

void cairo_demo(uint32_t *framebuffer, uint32_t width, uint32_t height);

static void draw_demo(void) {
    if (driver_state != DISPLAY_ENABLED) {
        return;
    }

    TRACE("drawing a demo image");
    cairo_demo(framebuffer, display_width, display_height);
    // for (uint32_t y = 0; y < display_height; y++) {
    //     for (uint32_t x = 0; x < display_width; x++) {
    //         framebuffer[y * display_width + x] = pick_color(y >> 4);
    //     }
    // }

    start_flushing_scanout();
}
#endif

void main(void) {
    init();

#ifdef CONFIG_VIRTIO_GPU_DEMO
    timer_set(1000);
#endif

    INFO("ready");
    struct message m;
    while (true) {
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG:
                if (m.notifications.data & NOTIFY_IRQ) {
                    handle_interrupt();
                }
                break;
            case GPU_SET_DEFAULT_MODE_MSG:
                m.type = GPU_SET_DEFAULT_MODE_REPLY_MSG;
                m.gpu_set_default_mode_reply.num_buffers = 1;
                m.gpu_set_default_mode_reply.width = display_width;
                m.gpu_set_default_mode_reply.height = display_height;
                ipc_reply(m.src, &m);
                break;
            case GPU_GET_BUFFER_MSG:
                m.type = GPU_GET_BUFFER_REPLY_MSG;
                m.gpu_get_buffer_reply.shm_handle = shm_handle;
                ipc_reply(m.src, &m);
                break;
            case GPU_SWITCH_FRONT_BUFFER_MSG:
                m.type = GPU_SWITCH_FRONT_BUFFER_REPLY_MSG;
                switch_front_buffer(m.gpu_switch_front_buffer.index);
                ipc_reply(m.src, &m);
                break;
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
