#ifndef __GUEST_H__
#define __GUEST_H__

#include <types.h>
#include <list.h>

struct guest {
    list_elem_t next;
    task_t task;
    uint64_t entry;
    uint64_t *ept_pml4;
    paddr_t ept_pml4_paddr;
    list_t mappings;
    list_t page_areas;
    unsigned pending_irq_bitmap;
    bool halted;
    struct {
        uint64_t rbx;
    } initial_regs;
    struct {
        list_t devices;
        int next_slot;
        int next_irq;
        uint16_t next_port_base;
        uint32_t selected_addr;
        bool read_bar0_size;
    } pci;
};

enum image_type {
    IMAGE_TYPE_PLAIN,
    IMAGE_TYPE_XEN_PVH,
};

#define E820_TYPE_RAM           1
#define E820_TYPE_RESERVED      2

struct __packed pvh_memmap_entry {
    uint64_t addr;
    uint64_t size;
    /// A E820 type.
    uint32_t type;
    uint32_t reserved;
};

struct __packed pvh_start_info {
    uint32_t magic;
    uint32_t version;
    uint32_t flags;
    uint32_t num_module_entries;
    uint64_t modules_paddr;
    uint64_t cmdline_paddr;
    uint64_t rsdp_paddr;
    uint64_t memmap_paddr;
    uint32_t num_memmap_entries;
    uint32_t reserved;
    struct {
        char cmdline[128];
        struct pvh_memmap_entry memmap[3];
    } data;
};

struct guest *guest_lookup(task_t task);
error_t guest_create(uint8_t *elf, size_t elf_len);
void guest_inject_irq(struct guest *guest, int irq);
void guest_tick(void);
void guest_send_pending_events(void);
void guest_init(void);

#endif
