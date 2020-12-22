#ifndef __MM_H__
#define __MM_H__

#include <types.h>
#include <list.h>

struct page_area {
    list_elem_t next;
    paddr_t paddr;
    vaddr_t vaddr;
    gpaddr_t gpaddr;
    size_t num_pages;
    bool internal;
};

struct mapping_ops;
struct mapping {
    list_elem_t next;
    gpaddr_t gpaddr;
    size_t len;
    struct mapping_ops *ops;
    union {
        struct {
            uint8_t *data;
        } file;
    };
};

struct mapping_ops {
    const char *name;
    error_t (*fill_page)(struct guest *guest, struct mapping *mapping, uint8_t *buf,
                         gpaddr_t gpaddr, size_t *filled_len);
    uint64_t (*read)(struct guest *guest, struct mapping *mapping,
                     offset_t offset, int size);
    void (*write)(struct guest *guest, struct mapping *mapping,
                  offset_t offset, int size, uint64_t value);
};

#define MP_FLOATPTR_SIGNATURE   0x5f504d5f /* "_MP_" */
struct __packed mp_float_ptr {
    uint32_t signature;
    uint32_t mptable_header_addr;
    uint8_t length;
    uint8_t spec;
    uint8_t checksum;
    uint8_t info1;
    uint8_t info2;
    uint8_t info3[3];
};

struct __packed mp_table_header {
    uint32_t signature;
    uint16_t base_table_length;
    uint8_t spec;
    uint8_t checksum;
    uint8_t oem_id[8];
    uint8_t product_id[12];
    uint32_t oem_table_pointer;
    uint16_t oem_table_size;
    uint16_t entry_count;
    uint32_t localapic_addr;
    uint16_t extended_table_length;
    uint8_t extended_table_checksum;
    uint8_t reserved;
};

struct __packed mp_processor_entry {
    uint8_t type;  // 0
    uint8_t localapic_id;
    uint8_t localapic_ver;
    uint8_t cpu_flags;
    uint32_t cpu_signature;
    uint32_t feature_flags;
    uint32_t reserved1;
    uint32_t reserved2;
};

#define MP_MPTABLE_SIGNATURE    0x504d4350 /* "PCMP" */
struct __packed misc_area {
    struct mp_float_ptr mp_float_ptr;
    struct mp_table_header mp_table_header;
    struct mp_processor_entry bsp_entry;
};

extern struct mapping_ops file_mapping_ops;
extern struct mapping_ops misc_mapping_ops;
extern struct mapping_ops lapic_mapping_ops;
extern struct mapping_ops pvh_mapping_ops;

void handle_ept_fault(struct guest *guest, gpaddr_t gpaddr, hv_frame_t *frame);
void *alloc_guest_page(struct guest *guest, size_t num_pages, gpaddr_t gpaddr,
                       paddr_t *paddr);
void *alloc_internal_page(struct guest *guest, size_t num_pages, paddr_t *paddr);
void *resolve_guest_paddr_buffer(struct guest *guest, gpaddr_t gpaddr);

#endif
