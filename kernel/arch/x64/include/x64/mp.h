#ifndef __X64_SMP_H__
#define __X64_SMP_H__

#include <types.h>

//
//  MP Table
//
#define MP_FLOATPTR_SIGNATURE 0x5f504d5f /* "_MP_" */
#define MP_MPTABLE_SIGNATURE 0x504d4350 /* "PCMP" */
#define MP_BASETABLE_PROCESSOR_ENTRY 0
#define MP_BASETABLE_BUS_ENTRY 1
#define MP_BASETABLE_IOAPIC_ENTRY 2
#define MP_BASETABLE_IOINT_ASSIGN_ENTRY 3
#define MP_BASETABLE_LOCALINT_ASSIGN_ENTRY 4

struct mp_float_ptr {
    uint32_t signature;
    uint32_t mptable_header_addr;
    uint8_t length;
    uint8_t spec_rev;
    uint8_t checksum;
    uint8_t info1;
    uint8_t info2;
    uint8_t info3[3];
} PACKED;

struct mp_table_header {
    uint32_t signature;
    uint16_t base_table_length;
    uint8_t spec_rev;
    uint8_t checksum;
    uint8_t oem_id[8];
    uint8_t product_id[12];
    uint32_t oem_table_pointer;
    uint16_t oem_table_size;
    uint16_t entry_count;
    uint32_t memmaped_localapic_addr;
    uint16_t extended_table_length;
    uint8_t extended_table_checksum;
    uint8_t reserved;
} PACKED;

struct mp_processor_entry {
    uint8_t type; // 0
    uint8_t localapic_id;
    uint8_t localapic_ver;
    uint8_t cpu_flags;
    uint32_t cpu_signature;
    uint32_t feature_flags;
    uint32_t reserved1;
    uint32_t reserved2;
} PACKED;

struct mp_bus_entry {
    uint8_t type; // 1
    uint8_t id;
    uint8_t type_str[6];
} PACKED;

struct mp_ioapic_entry {
    uint8_t type; // 2
    uint8_t ioapic_id;
    uint8_t ioapic_ver;
    uint8_t ioapic_flags;
    uint32_t memmaped_ioapic_addr;
} PACKED;

struct mp_ioint_assign_entry {
    uint8_t type; // 3
    uint8_t int_type;
    uint16_t int_flags;
    uint8_t src_bus_id;
    uint8_t src_bus_irq;
    uint8_t dest_ioapic_id;
    uint8_t dest_ioapic_intin;
} PACKED;

struct mp_localint_assign_entry {
    uint8_t type; // 4
    uint8_t int_type;
    uint16_t int_flags;
    uint8_t src_bus_id;
    uint8_t src_bus_irq;
    uint8_t dest_localapic_id;
    uint8_t dest_localapic_intin;
} PACKED;

void x64_read_mp_table(void);

#endif
