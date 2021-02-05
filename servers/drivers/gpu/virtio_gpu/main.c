#include "virtio_gpu.h"
#include <driver/dma.h>
#include <driver/io.h>
#include <driver/irq.h>
#include <endian.h>
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
static uint32_t *framebuffer = NULL;
enum driver_state driver_state = LOOKING_FOR_DISPLAY;

static void execute_command(struct virtio_gpu_ctrl_hdr *cmd, size_t len) {
    int req_index =
        virtio->virtq_alloc(ctrlq, len, VIRTQ_DESC_F_NEXT, VIRTQ_ALLOC_NO_PREV);
    int resp_index = virtio->virtq_alloc(ctrlq, VIRTIO_GPU_CTRLQ_ENTRY_MAX_SIZE,
                                         VIRTQ_DESC_F_WRITE, req_index);
    if (req_index < 0 || resp_index < 0) {
        WARN("failed to alloc a desc");
        return;
    }

    memcpy(virtio->get_buffer(ctrlq, req_index), cmd, len);
    virtio->virtq_kick_desc(ctrlq, resp_index);
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
    size_t len = display_height * display_width * sizeof(uint32_t);
    dma_t framebuffer_dma = dma_alloc(len, DMA_ALLOC_TO_DEVICE);
    framebuffer = (uint32_t *) dma_buf(framebuffer_dma);
    memset(framebuffer, 0 /* fill with black */, len);

    TRACE("attaching the framebuffer (%d KiB)", len / 1024);
    struct virtio_gpu_resource_attach_backing cmd;
    cmd.hdr.type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING;
    cmd.hdr.flags = 0;
    cmd.hdr.fence_id = 0;
    cmd.hdr.ctx_id = 0;
    cmd.hdr.padding = 0;
    cmd.resource_id = display_resource_id;
    cmd.nr_entries = 1;
    cmd.entries[0].addr = dma_daddr(framebuffer_dma);
    cmd.entries[0].length = len;
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
            TRACE("found a display: resolution=%dx%d", pmode->r.width, pmode->r.height);
            initialize_display(pmode->r.width, pmode->r.height);
            break;
        }
    }
}

static void handle_interrupt(void) {
    uint8_t status = virtio->read_isr_status();
    if (status & 1) {
        int index;
        size_t len;
        TRACE("IRQ");
        while (virtio->virtq_pop_desc(ctrlq, &index, &len) == OK) {
            int resp_index = virtio->get_next_index(ctrlq, index);
            TRACE("resp_index=%d", resp_index);
            struct virtio_gpu_ctrl_hdr *hdr = virtio->get_buffer(ctrlq, resp_index);
            switch (hdr->type) {
                case VIRTIO_GPU_RESP_OK_NODATA:
                    handle_ok_nodata();
                    break;
                case VIRTIO_GPU_RESP_OK_DISPLAY_INFO:
                    handle_display_info((struct virtio_gpu_resp_display_info *) hdr);
                    break;
                default:
                    WARN_DBG("unknown response from the device: type=0x%x", hdr->type);
            }
        }
    }
}

/// Looks for and initializes a virtio-gpu device.
static void init(void) {
    uint8_t irq;
    ASSERT_OK(virtio_find_device(VIRTIO_DEVICE_GPU, &virtio, &irq));
    virtio->negotiate_feature(0 /* No essential features for virito-gpu. */);

    virtio->virtq_init(VIRTIO_GPU_QUEUE_CTRL);
    virtio->virtq_init(VIRTIO_GPU_QUEUE_CURSOR);

    ctrlq = virtio->virtq_get(VIRTIO_GPU_QUEUE_CTRL);
    virtio->virtq_allocate_buffers(ctrlq, VIRTIO_GPU_CTRLQ_ENTRY_MAX_SIZE,
                                   false);

    ASSERT_OK(irq_acquire(irq));
    virtio->activate();

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
	    0xff0000,
	    0x800000,
	    0xffff00,
	    0x808000,
	    0x00ff00,
	    0x008000,
	    0x00ffff,
	    0x008080,
	    0x0000ff,
	    0x000080,
	    0xff00ff,
	    0x800080,
    };

    return colors[y % (sizeof(colors) / sizeof(uint32_t))];
}

static void draw_demo(void) {
    if (driver_state != DISPLAY_ENABLED) {
        return;
    }

    TRACE("drawing a demo image");
    for (uint32_t y =  0; y < display_height; y++) {
        for (uint32_t x = 0; x < display_width; x++) {
            framebuffer[y * display_width + x] = pick_color(y >> 4);
        }
    }

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

#ifdef CONFIG_VIRTIO_GPU_DEMO
                if (m.notifications.data & NOTIFY_TIMER) {
                    draw_demo();
                    timer_set(100);
                }
#endif

                break;
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
