#ifndef __DRIVER_PCI_H__
#define __DRIVER_PCI_H__

#define PCI_CONFIG_VENDOR_ID    0x00
#define PCI_CONFIG_DEVICE_ID    0x02
#define PCI_CONFIG_STATUS       0x06
#define PCI_CONFIG_NTH_BAR(n)   (0x10 + (n) * 4)
#define PCI_CONFIG_SUBVENDOR_ID 0x2c
#define PCI_CONFIG_SUBDEVICE_ID 0x2e
#define PCI_CONFIG_CAPABILITIES 0x34

// PCI status register flags.
#define PCI_CONFIG_STATUS_CAPS (1UL << 4)

// BAR fields.
#define PCI_BAR_IS_IO_MAPPED(bar)      (((bar) & 1) == 1)
#define PCI_BAR_IS_MEMORY_MAPPED(bar)  (!PCI_BAR_IS_IO_MAPPED(bar))
#define PCI_BAR_ADDR(bar)              ((bar) & 0xfffffff0)

// Capability fields.
#define PCI_CAP_VENDOR 0
#define PCI_CAP_NEXT   1
#define PCI_CAP_LEN    2

#endif
