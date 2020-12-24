#include "mm.h"
#include "guest.h"
#include "x64.h"
#include <list.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>

static error_t file_fill(struct guest *guest, struct mapping *m, uint8_t *buf,
                         gpaddr_t gpaddr, size_t *filled_len) {
    offset_t off = gpaddr - m->gpaddr;
    DEBUG_ASSERT(m->len >= off);

    size_t copy_len = MIN(PAGE_SIZE, m->len - off);
    memcpy(buf, &m->file.data[off], copy_len);

    // TRACE("ANON: gp=%x, off=%x, len=%d, buf=[%02x %02x %02x %02x]",
    //       gpaddr, off, copy_len, buf[0], buf[1], buf[2], buf[3]);

    *filled_len = copy_len;
    return OK;
}

struct mapping_ops file_mapping_ops = {
    .name = "file",
    .fill_page = file_fill,
    .read = NULL,
    .write = NULL,
};

static struct misc_area misc_area = {
    .mp_float_ptr =
        {
            .signature = MP_FLOATPTR_SIGNATURE,
            .spec = 4,
            .length = 1,
        },
    .mp_table_header =
        {
            .signature = MP_MPTABLE_SIGNATURE,
            .spec = 4,
            .base_table_length = sizeof(struct mp_table_header)
                                 + sizeof(struct mp_processor_entry),
            .localapic_addr = 0xfee00000,
        },
    .bsp_entry = {
        .type = 0,  // Processor entry
        .cpu_flags = (1u << 1) /* BSP */ | (1u << 0) /* Enabled */,
    }};

STATIC_ASSERT(sizeof(misc_area) < PAGE_SIZE);

/// "A checksum of the complete pointer structure. All bytes specified by the
/// length field, including CHECKSUM and reserved bytes, must add up to zero. "
///
/// -- 4.1 MP Floating Pointer Structure, MultiProcessor Specification Ver. 1.4
static uint8_t compute_mp_checksum(uint8_t *buf, size_t len) {
    // Use uint64_t for a workaround for UBSan overflow detectors.
    // TODO: Add an attirbute to disable the check.
    uint64_t sum = 0;
    while (len-- > 0) {
        sum += *buf++;
    }
    return ~(sum & 0xff) + 1;
}

static error_t misc_fill(struct guest *guest, struct mapping *m, uint8_t *buf,
                         gpaddr_t gpaddr, size_t *filled_len) {
    struct mp_float_ptr *fp = &misc_area.mp_float_ptr;
    struct mp_table_header *header = &misc_area.mp_table_header;
    ASSERT(fp->checksum == 0 && "why it's called twice?");

    fp->mptable_header_addr =
        gpaddr + offsetof(struct misc_area, mp_table_header);
    misc_area.mp_float_ptr.checksum =
        compute_mp_checksum((uint8_t *) fp, sizeof(*fp));
    header->checksum = compute_mp_checksum(
        (uint8_t *) header,
        sizeof(*header) + sizeof(struct mp_processor_entry));

    memcpy(buf, &misc_area, sizeof(misc_area));
    *filled_len = sizeof(misc_area);
    return OK;
}

struct mapping_ops misc_mapping_ops = {
    .name = "misc",
    .fill_page = misc_fill,
    .read = NULL,
    .write = NULL,
};

static uint64_t lapic_read(struct guest *guest, struct mapping *mapping,
                           offset_t offset, int size) {
    switch (offset) {
        // Local APIC ID Register
        case 0x20:
            return 0;
        // Local APIC Version Register
        case 0x30:
            return 0;
        default:
            WARN_DBG("lapic_read: not implemented register: offset=%x", offset);
            return 0;
    }
}

static void lapic_write(struct guest *guest, struct mapping *mapping,
                        offset_t offset, int size, uint64_t value) {
    switch (offset) {
        case 0x320:
            break;
        default:
            WARN_DBG("lapic_write: not implemented register: offset=%x",
                     offset);
    }
}

struct mapping_ops lapic_mapping_ops = {
    .name = "lapic",
    .fill_page = NULL,
    .read = lapic_read,
    .write = lapic_write,
};

struct pvh_start_info pvh_start_info = {
    .magic = 0x336ec578,
    .version = 1,
    .flags = 0,
    .num_module_entries = 0,
    .modules_paddr = 0,
    .cmdline_paddr = 0,
    .rsdp_paddr = 0,
    .memmap_paddr = 0,
    .num_memmap_entries = 0,
    .reserved = 0,
};

static error_t pvh_fill(struct guest *guest, struct mapping *m, uint8_t *buf,
                        gpaddr_t gpaddr, size_t *filled_len) {
    strncpy(pvh_start_info.data.cmdline, "console=ttyS0 root=/dev/vda1",
            sizeof(pvh_start_info.data.cmdline));

    pvh_start_info.num_memmap_entries = 3;
    pvh_start_info.data.memmap[0].addr = 0x00000000;
    pvh_start_info.data.memmap[0].size = 0x00008000;
    pvh_start_info.data.memmap[0].type = E820_TYPE_RESERVED;
    pvh_start_info.data.memmap[1].addr = 0x00008000;
    pvh_start_info.data.memmap[1].size = 0x00008000;
    pvh_start_info.data.memmap[1].type = E820_TYPE_RAM;
    pvh_start_info.data.memmap[1].reserved = 0;
    pvh_start_info.data.memmap[2].addr = 0x01000000;
    pvh_start_info.data.memmap[2].size =
        0x04000000; /* 64MiB; TODO: expand dynamically */
    pvh_start_info.data.memmap[2].type = E820_TYPE_RAM;
    pvh_start_info.data.memmap[2].reserved = 0;

    pvh_start_info.cmdline_paddr =
        gpaddr + offsetof(struct pvh_start_info, data.cmdline);
    pvh_start_info.memmap_paddr =
        gpaddr + offsetof(struct pvh_start_info, data.memmap);

    memcpy(buf, &pvh_start_info, sizeof(pvh_start_info));
    *filled_len = sizeof(pvh_start_info);
    return OK;
}

struct mapping_ops pvh_mapping_ops = {
    .name = "pvh",
    .fill_page = pvh_fill,
};

static struct page_area *resolve_paddr(struct guest *guest, paddr_t paddr) {
    LIST_FOR_EACH (pa, &guest->page_areas, struct page_area, next) {
        if (pa->paddr <= paddr
            && paddr < pa->paddr + pa->num_pages * PAGE_SIZE) {
            return pa;
        }
    }

    return NULL;
}

static struct page_area *resolve_guest_paddr(struct guest *guest,
                                             gpaddr_t gpaddr) {
    LIST_FOR_EACH (pa, &guest->page_areas, struct page_area, next) {
        if (!pa->internal && pa->gpaddr <= gpaddr
            && gpaddr < pa->gpaddr + pa->num_pages * PAGE_SIZE) {
            return pa;
        }
    }

    return NULL;
}

static vaddr_t paddr2ptr(struct guest *guest, paddr_t paddr) {
    struct page_area *pa = resolve_paddr(guest, paddr);
    ASSERT(pa != NULL);
    return pa->vaddr + (paddr - pa->paddr);
}

#define ENTRY_PADDR(entry) ((entry) &0x0000fffffffff000)
#define NTH_LEVEL_INDEX(level, vaddr)                                          \
    (((vaddr) >> ((((level) -1) * 9) + 12)) & 0x1ff)

static uint64_t *ept_traverse(struct guest *guest, gpaddr_t gpaddr,
                              uint64_t attrs, bool fill_if_not_exists) {
    ASSERT(IS_ALIGNED(gpaddr, PAGE_SIZE));

    uint64_t *table = guest->ept_pml4;
    for (int level = 4; level > 1; level--) {
        int index = NTH_LEVEL_INDEX(level, gpaddr);
        if (!table[index]) {
            if (!fill_if_not_exists) {
                return NULL;
            }

            paddr_t new_table_paddr;
            void *new_table = alloc_internal_page(guest, 1, &new_table_paddr);
            memset(new_table, 0, PAGE_SIZE);
            table[index] = new_table_paddr;
        }

        table[index] |= attrs;
        table = (uint64_t *) paddr2ptr(guest, ENTRY_PADDR(table[index]));
    }

    uint64_t *entry = &table[NTH_LEVEL_INDEX(1, gpaddr)];
    if (!fill_if_not_exists && !*entry) {
        return NULL;
    }

    return entry;
}

void handle_ept_fault(struct guest *guest, gpaddr_t gpaddr, hv_frame_t *frame);
void *resolve_guest_paddr_buffer(struct guest *guest, gpaddr_t gpaddr) {
    gpaddr_t aligned_gpaddr = ALIGN_DOWN(gpaddr, PAGE_SIZE);
    offset_t offset = gpaddr % PAGE_SIZE;

    struct page_area *pa = resolve_guest_paddr(guest, aligned_gpaddr);
    if (!pa) {
        // The guest physical memory page does not exist.
        handle_ept_fault(guest, aligned_gpaddr, NULL);
        pa = resolve_guest_paddr(guest, aligned_gpaddr);
        ASSERT(pa != NULL);
    }
    return (void *) (pa->vaddr + offset);
}

#define PAGE_P  (1u << 0)
#define PAGE_PS (1u << 7)
static gpaddr_t resolve_guest_vaddr(struct guest *guest, uint64_t cr3,
                                    uint64_t gvaddr) {
    // TODO: Support PML4
    uint64_t guest_pml4 = cr3 & 0x7ffffffffffff000;
    uint64_t *table = resolve_guest_paddr_buffer(guest, guest_pml4);
    for (int level = 4; level > 1; level--) {
        int index = NTH_LEVEL_INDEX(level, gvaddr);
        if (!table[index]) {
            return 0;
        }

        if ((table[index] & PAGE_PS) != 0 && ((table[index]) & PAGE_P) != 0) {
            uint64_t offset = gvaddr & ((1ul << (13 + 8 * (level - 1))) - 1);
            return ENTRY_PADDR(table[index]) + offset;
        }

        table = resolve_guest_paddr_buffer(guest, ENTRY_PADDR(table[index]));
    }

    uint64_t *entry = &table[NTH_LEVEL_INDEX(1, gvaddr)];
    if (((*entry) & PAGE_P) != 0) {
        return 0;
    }

    uint64_t offset = gvaddr & 0xffff;
    return ENTRY_PADDR(*entry) + offset;
}

static void *alloc_page(struct guest *guest, size_t num_pages, gpaddr_t gpaddr,
                        bool internal, paddr_t *paddr) {
    struct message m;
    m.type = VM_ALLOC_PAGES_MSG;
    m.vm_alloc_pages.paddr = 0;
    m.vm_alloc_pages.num_pages = num_pages;
    error_t err = ipc_call(INIT_TASK, &m);
    ASSERT_OK(err);
    ASSERT(m.type == VM_ALLOC_PAGES_REPLY_MSG);
    *paddr = m.vm_alloc_pages_reply.paddr;

    struct page_area *pa = malloc(sizeof(*pa));
    pa->vaddr = m.vm_alloc_pages_reply.vaddr;
    pa->paddr = m.vm_alloc_pages_reply.paddr;
    pa->gpaddr = gpaddr;
    pa->num_pages = num_pages;
    pa->internal = internal;
    list_push_back(&guest->page_areas, &pa->next);

    return (void *) pa->vaddr;
}

void *alloc_guest_page(struct guest *guest, size_t num_pages, gpaddr_t gpaddr,
                       paddr_t *paddr) {
    return alloc_page(guest, num_pages, gpaddr, false, paddr);
}

void *alloc_internal_page(struct guest *guest, size_t num_pages,
                          paddr_t *paddr) {
    return alloc_page(guest, num_pages, 0, true, paddr);
}

static error_t handle_mmio_access(struct guest *guest, struct mapping *mapping,
                                  gpaddr_t gpaddr, hv_frame_t *frame) {
    // TODO: Check if the paging is enabled.

    // Guest's virtual address -> Guest's physical address.
    uint64_t rip_gpaddr = resolve_guest_vaddr(guest, frame->cr3, frame->rip);
    if (!rip_gpaddr) {
        WARN_DBG("handle_mmio_access: guest-unmapped RIP address: %p",
                 frame->rip);
        return ERR_NOT_FOUND;
    }

    // Guest's physical address -> Host's physical address.
    uint64_t *host_entry =
        ept_traverse(guest, ALIGN_DOWN(rip_gpaddr, PAGE_SIZE), 0, false);
    if (!host_entry) {
        WARN_DBG("handle_mmio_access: host-unmapped RIP address: %p",
                 rip_gpaddr);
        return ERR_NOT_FOUND;
    }

    paddr_t rip_paddr = ENTRY_PADDR(*host_entry);
    struct page_area *pa = resolve_paddr(guest, rip_paddr);
    ASSERT(pa != NULL);

    // Finally we have a valid pointer to access the RIP...
    uint8_t *inst = (uint8_t *) pa->vaddr + (rip_gpaddr - pa->gpaddr);
    if (ALIGN_DOWN((vaddr_t) inst, PAGE_SIZE)
        != ALIGN_DOWN((vaddr_t) inst + frame->inst_len, PAGE_SIZE)) {
        // TODO:
        WARN_DBG(
            "handle_mmio_access: the instruction is in multiple pages (guest_rip=%p)",
            frame->rip);
        return ERR_NOT_ACCEPTABLE;
    }

    // Decode the instruction.
    enum x64_op op;
    bool reg_to_mem;
    enum x64_reg reg;
    int size;
    if (decode_inst(inst, frame->inst_len, &op, &reg, &reg_to_mem, &size)
        != OK) {
        WARN_DBG("unsupported (or invalid) instruction (guest_rip=%p)",
                 frame->rip);
        return ERR_NOT_ACCEPTABLE;
    }

    ASSERT(size == 4 && "only 32-bit mmio access is implemented");

    // Emulate the instruction.
    // TRACE("emulate memory %s: reg=%d, gpaddr=%p",
    //     reg_to_mem ? "write" : "read",  reg, gpaddr);
    offset_t offset = gpaddr - mapping->gpaddr;
    if (reg_to_mem) {
        uint64_t value;
        switch (reg) {
            case X64_REG_EAX:
                value = frame->rax & 0xffffffff;
                break;
            case X64_REG_ECX:
                value = frame->rcx & 0xffffffff;
                break;
            case X64_REG_EDX:
                value = frame->rdx & 0xffffffff;
                break;
            case X64_REG_EBX:
                value = frame->rbx & 0xffffffff;
                break;
            case X64_REG_ESI:
                value = frame->rsi & 0xffffffff;
                break;
            case X64_REG_EDI:
                value = frame->rdi & 0xffffffff;
                break;
        }
        mapping->ops->write(guest, mapping, offset, value, size);
    } else {
        uint64_t value = mapping->ops->read(guest, mapping, offset, size);
        switch (reg) {
            case X64_REG_EAX:
                frame->rax = value & 0xffffffff;
                break;
            case X64_REG_ECX:
                frame->rcx = value & 0xffffffff;
                break;
            case X64_REG_EDX:
                frame->rdx = value & 0xffffffff;
                break;
            case X64_REG_EBX:
                frame->rbx = value & 0xffffffff;
                break;
            case X64_REG_ESI:
                frame->rsi = value & 0xffffffff;
                break;
            case X64_REG_EDI:
                frame->rdi = value & 0xffffffff;
                break;
        }
    }

    return OK;
}

void handle_ept_fault(struct guest *guest, gpaddr_t gpaddr, hv_frame_t *frame) {
    gpaddr_t aligned_gpaddr = ALIGN_DOWN(gpaddr, PAGE_SIZE);
    struct mapping *mapping = NULL;
    LIST_FOR_EACH (m, &guest->mappings, struct mapping, next) {
        if (m->gpaddr <= gpaddr && gpaddr < m->gpaddr + m->len) {
            mapping = m;
            break;
        }
    }

    paddr_t paddr;
    uint8_t *page = alloc_guest_page(guest, 1, aligned_gpaddr, &paddr);
    size_t filled_len = 0;
    bool map_page;
    if (mapping) {
        if (mapping->ops->read || mapping->ops->write) {
            if (!frame) {
                WARN_DBG(
                    "accessing mmio with frame=NULL, filling a zeroed page for gpaddr=%p",
                    gpaddr);
                map_page = true;
            } else {
                map_page = false;
                DEBUG_ASSERT(mapping->ops->read != NULL
                             && mapping->ops->write != NULL);
                ASSERT_OK(handle_mmio_access(guest, mapping, gpaddr, frame));
                frame->rip += frame->inst_len;
            }
        } else {
            map_page = true;
            DEBUG_ASSERT(mapping->ops->fill_page != NULL);
            ASSERT_OK(mapping->ops->fill_page(guest, mapping, page,
                                              aligned_gpaddr, &filled_len));
        }
        // TRACE("%s_fill: gpaddr=%p, filled_len=%p", mapping->ops->name,
        // gpaddr, filled_len);
    } else {
        map_page = true;
        // TRACE("anon_fill: gpaddr=%p", gpaddr);
    }

    if (map_page) {
        DEBUG_ASSERT(filled_len <= PAGE_SIZE);
        memset(&page[filled_len], 0, PAGE_SIZE - filled_len);

        uint64_t ept_attrs = 0x07;  // allow RWX access
        uint64_t *entry = ept_traverse(guest, aligned_gpaddr, ept_attrs, true);
        *entry = paddr | ept_attrs;
    }
}
