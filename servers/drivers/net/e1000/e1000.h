#ifndef __E1000_H__
#define __E1000_H__

#include <list.h>
#include <types.h>

/// Controls the device features and states.
#define REG_CTRL 0x0000
/// Auto-Speed Detetion Enable.
#define CTRL_ASDE (1 << 5)
/// Set link up.
#define CTRL_SLU (1 << 6)
/// Device Reset.
#define CTRL_RST (1 << 26)

/// Interrupt Mask Set.
#define REG_IMS  0x00d0
#define IMS_RXT0 (1 << 7)

/// Interrupt Mask Clear.
#define REG_IMC 0x00d8

/// Interrupt Cause Read.
#define REG_ICR 0x00c0
/// Receiver Timer Interrupt.
#define ICR_RXT0 (1 << 7)

/// Multicast Table Array.
#define REG_MTA_BASE 0x5200
/// The lower bits of the Ethernet address.
#define REG_RECEIVE_ADDR_LOW 0x5400
/// The higher bits of the Ethernet address and some extra bits.
#define REG_RECEIVE_ADDR_HIGH 0x5404

/// Receive Control.
#define REG_RCTL 0x0100
/// Receiver Enable.
#define RCTL_EN (1 << 1)
/// Strip Ethernet CRC from receiving packet.
#define RCTL_SECRC (1 << 26)
/// Receive Buffer Size: 2048 bytes (assuming RCTL.BSEX == 0).
#define RCTL_BSIZE 0 << 16
/// Broadcast Accept Mode.
#define RCTL_BAM (1 << 15)

/// Receive Descriptor Base Low.
#define REG_RDBAL 0x2800
/// Receive Descriptor Base High.
#define REG_RDBAH 0x2804
/// Length of Receive Descriptors.
#define REG_RDLEN 0x2808
/// Receive Descriptor Head.
#define REG_RDH 0x2810
/// Receive Descriptor Tail.
#define REG_RDT 0x2818

/// Transmit Control.
#define REG_TCTL 0x0400
/// Receiver Enable.
#define TCTL_EN (1 << 1)
/// Pad Short Packets.
#define TCTL_PSP (1 << 3)

/// Transmit Descriptor Base Low.
#define REG_TDBAL 0x3800
/// Transmit Descriptor Base High.
#define REG_TDBAH 0x3804
/// Length of Transmit Descriptors.
#define REG_TDLEN 0x3808
/// Transmit Descriptor Head.
#define REG_TDH 0x3810
/// Transmit Descriptor Tail.
#define REG_TDT 0x3818

/// The size of buffer to store received/transmtting packets.
#define BUFFER_SIZE 2048
/// Number of receive descriptors.
#define NUM_RX_DESCS 32
/// Number of receive descriptors.
#define NUM_TX_DESCS 32

struct rx_desc {
    uint64_t paddr;
    uint16_t len;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __packed;

/// Descriptor Done.
#define RX_DESC_DD (1 << 0)
/// End Of Packet.
#define RX_DESC_EOP (1 << 1)

struct tx_desc {
    uint64_t paddr;
    uint16_t len;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __packed;

/// Insert FCS.
#define TX_DESC_IFCS (1 << 1)
/// End Of Packet.
#define TX_DESC_EOP (1 << 0)

/// A packet buffer.
struct buffer {
    uint8_t data[BUFFER_SIZE];
} __packed;

/// A received packet.
struct rx_packet {
    list_elem_t next;
    size_t len;
    uint8_t payload[BUFFER_SIZE];
};

struct pci_device;
void e1000_init_for_pci(uint32_t bar0_addr, uint32_t bar0_len);
void e1000_transmit(const void *pkt, size_t len);
void e1000_handle_interrupt(void (*receive)(const void *payload, size_t len));
void e1000_read_macaddr(uint8_t *macaddr);

#endif
