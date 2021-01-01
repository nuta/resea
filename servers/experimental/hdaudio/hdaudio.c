#include "hdaudio.h"
#include <driver/dma.h>
#include <driver/io.h>
#include <resea/ipc.h>
#include <resea/printf.h>
#include <string.h>

static io_t regs;
static dma_t corb_dma;
static dma_t rirb_dma;
static uint32_t corb_wp = 1;
static int corb_num_entries;
static int rirb_num_entries;
static int gcap_iss = 0;
static int gcap_oss = 0;
static int speaker_node = -1;
static int pin_node = -1;
static struct hdaudio_stream out_stream;

/// Appends an entry to CORB. Returns its index.
static uint32_t corb_enqueue(int cad, uint32_t nid, uint32_t verb,
                             uint32_t payload) {
    uint32_t *queue = (uint32_t *) dma_buf(corb_dma);
    uint32_t current = corb_wp % corb_num_entries;
    uint32_t data = (cad << 28) | (nid << 20) | (verb << 8) | payload;
    queue[current] = data;
    io_write16(regs, REG_CORBWP, current);
    corb_wp++;
    return current;
}

/// Returns the `index`-th RIRB data, in other words, the response for the
/// `index`-th request in CORB.
static uint32_t rirb_data(uint32_t index) {
    uint32_t *resps = (uint32_t *) dma_buf(rirb_dma);
    return resps[index * 2];
}

/// Requests a command to the specified node and returns its response. `cad` and
/// `nid` is the node address (NID). The codec address (CAd) is fixed to 0 for
/// now.
static uint32_t run_command(uint32_t nid, uint32_t verb, uint32_t payload) {
    int cad = 0;
    uint32_t idx = corb_enqueue(cad, nid, verb, payload);
    while ((io_read8(regs, REG_RIRBSTS) & 1) == 0)
        ;
    io_write8(regs, REG_RIRBSTS, 0x5);
    return rirb_data(idx);
}

/// Initializes the Command Outbound Ring Buffer (CORB).
static void init_corb(void) {
    size_t corb_len = PAGE_SIZE;
    corb_dma = dma_alloc(corb_len, DMA_ALLOC_FROM_DEVICE);

    // Stop CORB.
    io_write8(regs, REG_CORBCTL,
              io_read8(regs, REG_CORBCTL) | ~CORBCTL_CORBRUN);

    // TODO: Check and specify available # of entries.
    ASSERT((io_read8(regs, REG_CORBSIZE) & 0b11) == 0b10);
    corb_num_entries = 256;

    io_write32(regs, REG_CORBLBASE, dma_daddr(corb_dma));
    io_write32(regs, REG_CORBHBASE, dma_daddr(corb_dma) >> 32);
    io_write16(regs, REG_CORBWP, 0);

    // Read Pointer Reset.
    io_write16(regs, REG_CORBRP, 1 << 15);
    io_write16(regs, REG_CORBRP, 0);

    // Enable CORB.
    io_write8(regs, REG_CORBCTL, io_read8(regs, REG_CORBCTL) | CORBCTL_CORBRUN);
}

/// Initializes Response Inbound Ring Buffer (RIRB).
static void init_rirb(void) {
    size_t rirb_len = PAGE_SIZE;
    rirb_dma = dma_alloc(rirb_len, DMA_ALLOC_FROM_DEVICE);

    // Stop RIRB.
    io_write8(regs, REG_RIRBCTL,
              io_read16(regs, REG_RIRBCTL) & ~RIRBCTL_RIRBDMAEN);

    // TODO: Check and specify available # of entries.
    ASSERT((io_read8(regs, REG_RIRBSIZE) & 0b11) == 0b10);
    rirb_num_entries = 256;

    io_write32(regs, REG_RIRBLBASE, dma_daddr(rirb_dma));
    io_write32(regs, REG_RIRBHBASE, dma_daddr(rirb_dma) >> 32);

    io_write16(regs, REG_RINTCNT, 1);
    io_write16(regs, REG_RIRBWP, 1 << 15 /* Reset */);

    io_write8(
        regs, REG_RIRBCTL,
        io_read16(regs, REG_RIRBCTL) | RIRBCTL_RIRBDMAEN | RIRBCTL_RINTCTL);
}

/// Looks for a node with the given widget type.
static int look_for_node(uint8_t parent_nid, uint8_t target_widget_type) {
    uint32_t value = run_command(parent_nid, VERB_GET_PARAM, PARAM_NODE_CNT);
    uint32_t nid_base = (value >> 16) & 0xff;
    uint32_t num_nodes = value & 0xff;
    if (!num_nodes) {
        return -1;
    }

    for (uint32_t i = 0; i < num_nodes; i++) {
        uint32_t value =
            run_command(nid_base + i, VERB_GET_PARAM, PARAM_AUDIO_WIDGET_CAPS);
        uint8_t widget_type = (value >> 20) & 0xf;
        if (widget_type == target_widget_type) {
            return nid_base + i;
        }

        int nid = look_for_node(nid_base + i, target_widget_type);
        if (nid >= 0) {
            return nid;
        }
    }

    return -1;
}

/// Initializes a stream.
static void init_out_stream(struct hdaudio_stream *stream, int id) {
    ASSERT(id > 0);
    stream->id = id;
    stream->offset = 0x80 + gcap_iss * 0x20 + (id - 1) * 0x20;
    stream->buffer_size = 512 * 1024 /* 512KiB */ * 2;
    stream->buffer_dma = dma_alloc(stream->buffer_size, DMA_ALLOC_FROM_DEVICE);

    // Set the stream ID.
    io_write32(regs, REG_SDnCTL(stream),
               (io_read32(regs, REG_SDnCTL(stream)) & ~(0xf << 20))
                   | (stream->id << 20));

    // Populate the buffer descriptor list.
    stream->bdl_dma = dma_alloc(stream->buffer_size, DMA_ALLOC_FROM_DEVICE);
    struct hdaudio_buffer_desc *bd_list =
        (struct hdaudio_buffer_desc *) dma_buf(stream->bdl_dma);

    // The spec requires at least two entries (see "3.3.39 Offset 8C:
    // {IOB}ISDnLVI" in the spec).
    size_t num_bds = 2;
    bd_list[0].buffer_addr = dma_daddr(stream->buffer_dma);
    bd_list[0].buffer_len = stream->buffer_size;
    bd_list[0].ioc = 0;
    bd_list[1].buffer_addr = 0;
    bd_list[1].buffer_len = 0;
    bd_list[1].ioc = 0;

    io_write32(regs, REG_SDnCBL(stream), stream->buffer_size);
    io_write32(regs, REG_SDnLVI(stream), num_bds - 1);
    io_write32(regs, REG_SDnBDLPL(stream), dma_daddr(stream->bdl_dma));
    io_write32(regs, REG_SDnBDLPU(stream), dma_daddr(stream->bdl_dma) >> 32);
}

// Initializes the HD Audio controller.
void init_controller(void) {
    // Reset the controller.
    io_write32(regs, REG_GCTL, io_read32(regs, REG_GCTL) & ~GCTL_RESET);
    while ((io_read32(regs, REG_GCTL) & GCTL_RESET) == 1)
        ;
    io_write32(regs, REG_GCTL, GCTL_RESET);

    gcap_iss = (io_read16(regs, REG_GCAP) >> 8) & 0xf;
    gcap_oss = (io_read16(regs, REG_GCAP) >> 12) & 0xf;
    ASSERT(gcap_oss > 0 && "no output streams supported by the device");
    init_corb();
    init_rirb();
}

void init_speaker(uint16_t stream_format) {
    init_out_stream(&out_stream, 1);

    // Look for a pin widget.
    pin_node = look_for_node(NID_ROOT, WIDGET_TYPE_PIN);
    ASSERT(pin_node >= 0);
    TRACE("found pin widget: nid=%d", pin_node);

    // Get the # of connected widgets to the pin widget.
    uint32_t value = run_command(pin_node, VERB_GET_PARAM, PARAM_CON_LIST_LEN);
    int con_list_len = value & 0x07;
    bool is_long_form = (value & 0x80) != 0;
    ASSERT(!is_long_form && "long form is not supported");

    // Look for a speaker connected to the pin widget...
    speaker_node = -1;
    for (int i = 0; i * 4 < con_list_len; i++) {
        uint32_t value = run_command(pin_node, VERB_GET_CONLIST, i);
        for (int j = 0; i + j < con_list_len && j < 4; j++) {
            int nid = (value >> (8 * j)) & 0xff;
            TRACE("    connection #%d: entry=%d", i * 4 + j, nid);
            if (nid > 0) {
                speaker_node = nid;
                break;
            }
        }
    }

    ASSERT(speaker_node >= 0);
    TRACE("found a speaker widget: nid=%d", speaker_node);

    // Initialize the speaker node (strictly speaking, Audio Output Converter
    // Widget).
    run_command(speaker_node, VERB_SET_STREAM_FORMAT, stream_format);
    run_command(speaker_node, VERB_SET_CH_STREAM_ID, out_stream.id << 4);
    run_command(pin_node, VERB_SET_PIN_WIDGET_CTL, 0xc0);

    // Start the first stream descriptor. Maybe this is not needed.
    io_write32(regs, REG_SSYNC, 1);
}

void hdaudio_init(vaddr_t mmio_addr, size_t mmio_len, uint16_t stream_format) {
    // Map the MMIO area into our virtual memory space.
    regs = io_alloc_memory_fixed(mmio_addr, mmio_len, IO_ALLOC_CONTINUOUS);

    init_controller();
    init_speaker(stream_format);
}

void hdaudio_unmute(void) {
    run_command(speaker_node, VERB_SET_AMP_GAIN_MUTE,
                run_command(speaker_node, VERB_GET_AMP_GAIN_MUTE, 0) | 0xb000);
    run_command(pin_node, VERB_SET_POWER_STATE, 0x0000 /* D0: Fully On */);
}

void hdaudio_play(uint8_t *pcm_data, size_t len) {
    ASSERT(len <= out_stream.buffer_size);
    uint8_t *p = (uint8_t *) dma_buf(out_stream.buffer_dma);
    memcpy(p, pcm_data, len);

    uint32_t value = io_read32(regs, REG_SDnCTL(&out_stream));
    io_write32(regs, REG_SDnCTL(&out_stream), value | 0b10 /* Stream Run */);
}
