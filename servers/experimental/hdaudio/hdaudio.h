#ifndef __HDAUDUIO_H__
#define __HDAUDUIO_H__

#include <driver/dma.h>
#include <driver/io.h>
#include <types.h>

#define REG_GCAP      0x00
#define REG_GCTL      0x08
#define REG_SSYNC     0x38
#define REG_CORBLBASE 0x40
#define REG_CORBHBASE 0x44
#define REG_CORBWP    0x48
#define REG_CORBRP    0x4a
#define REG_CORBCTL   0x4c
#define REG_CORBSIZE  0x4e

#define REG_RIRBLBASE 0x50
#define REG_RIRBHBASE 0x54
#define REG_RIRBWP    0x58
#define REG_RINTCNT   0x5a
#define REG_RIRBCTL   0x5c
#define REG_RIRBSTS   0x5d
#define REG_RIRBSIZE  0x5e

#define REG_SDnCTL(stream)   ((stream)->offset + 0x00)
#define REG_SDnCBL(stream)   ((stream)->offset + 0x08)
#define REG_SDnLVI(stream)   ((stream)->offset + 0x0c)
#define REG_SDnFMT(stream)   ((stream)->offset + 0x12)
#define REG_SDnBDLPL(stream) ((stream)->offset + 0x18)
#define REG_SDnBDLPU(stream) ((stream)->offset + 0x1c)

#define VERB_GET_PARAM          0xf00
#define VERB_GET_CONLIST        0xf02
#define VERB_SET_STREAM_FORMAT  0x200
#define VERB_SET_CH_STREAM_ID   0x706
#define VERB_SET_PIN_WIDGET_CTL 0x707
#define VERB_GET_AMP_GAIN_MUTE  0xb00
#define VERB_SET_AMP_GAIN_MUTE  0x300
#define VERB_SET_POWER_STATE    0x705

/// Parameters that can be read by VERB_GET_PARAM.
#define PARAM_NODE_CNT          0x04
#define PARAM_AUDIO_WIDGET_CAPS 0x09
#define PARAM_CON_LIST_LEN      0x0e

/// GCTL fields.
#define GCTL_RESET (1 << 0)

// CORBCTL fields.
#define CORBCTL_CORBRUN (1 << 1)

// RIRBCTL fields.
#define RIRBCTL_RIRBDMAEN (1 << 1)
#define RIRBCTL_RINTCTL   (1 << 0)

/// The stream format parameters set by REG_SDnFMT.
#define FORMAT_441KHZ     (1 << 14)
#define FORMAT_BTS_8BITS  (0 << 4)
#define FORMAT_BTS_16BITS (1 << 4)
#define FORMAT_NUM_CH_2   (1 << 0)

#define NID_ROOT 0

/// "7.3.4.6. Audio Widget Capabilities"
#define WIDGET_TYPE_PIN 4

/// Buffer Descriptor List Entry.
struct __packed hdaudio_buffer_desc {
    uint64_t buffer_addr;
    uint32_t buffer_len;
    /// Interrupt on Completion.
    uint32_t ioc;
};

struct hdaudio_stream {
    int id;
    offset_t offset;
    dma_t buffer_dma;
    size_t buffer_size;
    dma_t bdl_dma;
};

void hdaudio_init(vaddr_t mmio_addr, size_t mmio_len, uint16_t stream_format);
void hdaudio_unmute(void);
void hdaudio_play(uint8_t *pcm_data, size_t len);

#endif
