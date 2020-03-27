#include <message.h>
#include <std/io.h>
#include <std/malloc.h>
#include <std/printf.h>
#include <std/syscall.h>
#include <cstring.h>
#include "e1000.h"
#include "pci.h"

static uintptr_t regs;
static uint32_t tx_current;
static uint32_t rx_current;
static struct rx_desc *rx_descs;
static struct tx_desc *tx_descs;
static struct buffer *rx_buffers;
static struct buffer *tx_buffers;

static uint32_t read_reg32(uint32_t offset) {
    memory_barrier();
    return *((volatile uint32_t *) (regs + offset));
}

static uint8_t read_reg8(uint32_t offset) {
    memory_barrier();
    uint32_t aligned_offset = offset & 0xfffffffc;
    uint32_t value = read_reg32(aligned_offset);
    return (value >> ((offset & 0x03) * 8)) & 0xff;
}

static void write_reg32(uint32_t offset, uint32_t value) {
    *((volatile uint32_t *) (regs + offset)) = value;
    memory_barrier();
}

void e1000_init(struct pci_device *pcidev) {
    // Map memory-mapped registers in our address space.
    int num_regs_pages = 8;
    paddr_t paddr;
    regs = (uintptr_t) io_alloc_pages(num_regs_pages, pcidev->bar0, &paddr);

    // Allocate memory pages for memory-mapped IO.
    paddr_t rx_descs_paddr;
    paddr_t tx_descs_paddr;
    paddr_t rx_buffers_paddr;
    paddr_t tx_buffers_paddr;
    rx_descs = io_alloc_pages(
        ALIGN_UP(sizeof(struct rx_desc) * NUM_RX_DESCS, PAGE_SIZE) / PAGE_SIZE,
        0, &rx_descs_paddr);
    tx_descs = io_alloc_pages(
        ALIGN_UP(sizeof(struct tx_desc) * NUM_TX_DESCS, PAGE_SIZE) / PAGE_SIZE,
        0, &tx_descs_paddr);
    rx_buffers = io_alloc_pages(
        ALIGN_UP(BUFFER_SIZE * NUM_RX_DESCS, PAGE_SIZE) / PAGE_SIZE, 0,
        &rx_buffers_paddr);
    tx_buffers = io_alloc_pages(
        ALIGN_UP(BUFFER_SIZE * NUM_TX_DESCS, PAGE_SIZE) / PAGE_SIZE, 0,
        &tx_buffers_paddr);

    // Reset the device.
    write_reg32(REG_CTRL, read_reg32(REG_CTRL) | CTRL_RST);

    // Wait until the device gets reset.
    while ((read_reg32(REG_CTRL) & CTRL_RST) != 0) {}

    // Link up!
    write_reg32(REG_CTRL, read_reg32(REG_CTRL) | CTRL_SLU | CTRL_ASDE);

    // Fill Multicast Table Array with zeros.
    for (int i = 0; i < 0x80; i++) {
        write_reg32(REG_MTA_BASE + i * 4, 0);
    }

    // Initialize RX queue.
    for (int i = 0; i < NUM_RX_DESCS; i++) {
        rx_descs[i].paddr = rx_buffers_paddr + i * BUFFER_SIZE;
        rx_descs[i].status = 0;
    }

    write_reg32(REG_RDBAL, rx_descs_paddr & 0xffffffff);
    write_reg32(REG_RDBAH, rx_descs_paddr >> 32);
    write_reg32(REG_RDLEN, NUM_RX_DESCS * sizeof(struct rx_desc));
    write_reg32(REG_RDH, 0);
    write_reg32(REG_RDT, NUM_RX_DESCS);
    write_reg32(REG_RCTL, RCTL_EN | RCTL_SECRC | RCTL_BSIZE | RCTL_BAM);

    // Initialize TX queue.
    for (int i = 0; i < NUM_TX_DESCS; i++) {
        tx_descs[i].paddr = tx_buffers_paddr + i * BUFFER_SIZE;
    }

    write_reg32(REG_TDBAL, tx_descs_paddr & 0xffffffff);
    write_reg32(REG_TDBAH, tx_descs_paddr >> 32);
    write_reg32(REG_TDLEN, NUM_TX_DESCS * sizeof(struct tx_desc));
    write_reg32(REG_TDH, 0);
    write_reg32(REG_TDT, 0);
    write_reg32(REG_TCTL, TCTL_EN | TCTL_PSP);

    // Enable interrupts.
    write_reg32(REG_IMS, 0xff);
    write_reg32(REG_IMC, 0xff);
    write_reg32(REG_IMS, IMS_RXT0);
}

void e1000_transmit(const void *pkt, size_t len) {
    ASSERT(len <= BUFFER_SIZE);

    // Copy the packet into the TX buffer.
    memcpy(tx_buffers[tx_current].data, pkt, len);

    // Set descriptor fields. The buffer address field is already filled
    // during initialization.
    struct tx_desc *desc = &tx_descs[tx_current];
    desc->cmd = TX_DESC_IFCS | TX_DESC_EOP;
    desc->len = len;
    desc->cso = 0;
    desc->status = 0;
    desc->css = 0;
    desc->special = 0;

    // Notify the device.
    tx_current = (tx_current + 1) % NUM_TX_DESCS;
    write_reg32(REG_TDT, tx_current);

    TRACE("sent %d bytes", len);
}

void e1000_handle_interrupt(void (*receive)(const void *payload, size_t len)) {
    uint32_t cause = read_reg32(REG_ICR);
    if ((cause & ICR_RXT0) != 0) {
        while (true) {
            struct rx_desc *desc = &rx_descs[rx_current];

            // We don't support a large packet which spans multiple descriptors.
            uint8_t bits = RX_DESC_DD | RX_DESC_EOP;
            if ((desc->status & bits) != bits) {
                break;
            }

            receive(rx_buffers[rx_current].data, desc->len);

            // Tell the device that we've tasked a received packet.
            rx_descs[rx_current].status = 0;
            write_reg32(REG_RDT, rx_current);
            rx_current = (rx_current + 1) % (NUM_RX_DESCS);
        }
    }
}

void e1000_read_macaddr(uint8_t *macaddr) {
    for (int i = 0; i < 4; i++) {
        macaddr[i] = read_reg8(REG_RECEIVE_ADDR_LOW + i);
    }

    for (int i = 0; i < 2; i++) {
        macaddr[4 + i] = read_reg8(REG_RECEIVE_ADDR_HIGH + i);
    }
}
