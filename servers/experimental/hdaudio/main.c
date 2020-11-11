#include "hdaudio.h"
#include <resea/ipc.h>
#include <resea/printf.h>
#include <driver/io.h>
#include <driver/dma.h>
#include <string.h>

// Contains singed short sound_data[].
#include "sound_data.h"

void main(void) {
    // Search the PCI bus for a sound device...
    task_t dm_server = ipc_lookup("dm");
    struct message m;
    m.type = DM_ATTACH_PCI_DEVICE_MSG;
    m.dm_attach_pci_device.vendor_id = 0x8086;
    m.dm_attach_pci_device.device_id = 0x2668;
    ASSERT_OK(ipc_call(dm_server, &m));
    handle_t pci_device = m.dm_attach_pci_device_reply.handle;

    m.type = DM_PCI_GET_CONFIG_MSG;
    m.dm_pci_get_config.handle = pci_device;
    ASSERT_OK(ipc_call(dm_server, &m));
    uint32_t bar0_addr = m.dm_pci_get_config_reply.bar0_addr;
    uint32_t bar0_len = m.dm_pci_get_config_reply.bar0_len;
    ASSERT((bar0_addr & 1) == 0 && "expected that BAR#0 is memory-mapped");

    // Enable PCI bus master.
    m.type = DM_PCI_ENABLE_BUS_MASTER_MSG;
    m.dm_pci_enable_bus_master.handle = pci_device;
    ASSERT_OK(ipc_call(dm_server, &m));

    // Play a sound (44.1kHz, 16-bits, 2 channels [stereo]).
    // PLEASE BE CAREFUL WITH YOUR SPEAKER VOLUME! IT COULD CAUSE A LOUD NOISE!!
    hdaudio_init(bar0_addr, bar0_len,
                 FORMAT_441KHZ | FORMAT_BTS_16BITS | FORMAT_NUM_CH_2);
    hdaudio_unmute();
    hdaudio_play((uint8_t *) sound_data, sizeof(sound_data));
}
