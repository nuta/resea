use super::ioapic;
use crate::arch::utils::PhyPtr;
use core::mem::size_of;
use core::option::Option;

const MP_BASETABLE_PROCESSOR_ENTRY: u8 = 0;
const MP_BASETABLE_BUS_ENTRY: u8 = 1;
const MP_BASETABLE_IOAPIC_ENTRY: u8 = 2;
const MP_BASETABLE_IOINT_ASSIGN_ENTRY: u8 = 3;
const MP_BASETABLE_LOCALINT_ASSIGN_ENTRY: u8 = 4;

#[repr(C, packed)]
struct FloatPtr {
    signature: [u8; 4], // "_MP_"
    mptable_header_addr: u32,
    length: u8,
    spec_rev: u8,
    checksum: u8,
    info1: u8,
    info2: u8,
    info3: [u8; 3],
}

#[repr(C, packed)]
struct MpTableHeader {
    signature: [u8; 4], // "PCMP"
    base_table_length: u16,
    spec_rev: u8,
    checksum: u8,
    oem_id: [u8; 8],
    product_id: [u8; 12],
    oem_table_pointer: u32,
    oem_table_size: u16,
    entry_count: u16,
    memmaped_localapic_addr: u32,
    extended_table_length: u16,
    extended_table_checksum: u8,
    reserved: u8,
}

#[repr(C, packed)]
struct MpProcessorEntry {
    id: u8,
    localapic_id: u8,
    localapic_ver: u8,
    cpu_flags: u8,
    cpu_signature: u32,
    feature_flags: u32,
    reserved1: u32,
    reserved2: u32,
}

#[repr(C, packed)]
struct MpBusEntry {
    id: u8,
    bus_id: u8,
    type_str: [u8; 6],
}

#[repr(C, packed)]
struct IoApicEntry {
    id: u8,
    ioapic_id: u8,
    ioapic_ver: u8,
    ioapic_flags: u8,
    memmaped_ioapic_addr: u32,
}

#[repr(C, packed)]
struct IoIntAssignEntry {
    id: u8,
    int_type: u8,
    int_flags: u16,
    src_bus_id: u8,
    src_bus_irq: u8,
    dest_ioapic_id: u8,
    dest_ioapic_intin: u8,
}

#[repr(C, packed)]
struct LocalIntAssignEntry {
    id: u8,
    int_type: u8,
    int_flags: u16,
    src_bus_id: u8,
    src_bus_irq: u8,
    dest_localapic_id: u8,
    dest_localapic_intin: u8,
}

/// Looks for MP Floating Pointer Table.
unsafe fn look_for_mp_float_ptr_table() -> Option<*const FloatPtr> {
    let mut p: PhyPtr<u8> = PhyPtr::new(0xf0000);
    for _ in 0..0x10000 {
        let float_ptr: *const FloatPtr = p.as_ptr();
        if (*float_ptr).signature[0] == '_' as u8
            && (*float_ptr).signature[1] == 'M' as u8
            && (*float_ptr).signature[2] == 'P' as u8
            && (*float_ptr).signature[3] == '_' as u8
        {
            return Some(p.as_ptr());
        }

        p = p.add(1);
    }

    None
}

pub unsafe fn init() {
    let float_ptr = look_for_mp_float_ptr_table().expect("failed to detect MP float ptr table");

    let table_header_paddr: PhyPtr<u8> = PhyPtr::new((*float_ptr).mptable_header_addr as usize);
    let table_header: *const MpTableHeader = table_header_paddr.as_ptr();
    if (*table_header).signature[0] != 'P' as u8
        || (*table_header).signature[1] != 'C' as u8
        || (*table_header).signature[2] != 'M' as u8
        || (*table_header).signature[3] != 'P' as u8
    {
        panic!("invalid MP table header");
    }

    let mut entry_ptr = table_header_paddr.vaddr_as_usize() + size_of::<MpTableHeader>();
    for _i in 0..(*table_header).entry_count {
        let entry_type = (entry_ptr as *const u8).read();
        let entry_size = match entry_type {
            MP_BASETABLE_IOAPIC_ENTRY => {
                let ioapic_entry = entry_ptr as *const IoApicEntry;
                if (*ioapic_entry).ioapic_flags != 0 {
                    ioapic::init((*ioapic_entry).memmaped_ioapic_addr as u64);
                }
                return;
            }
            MP_BASETABLE_BUS_ENTRY => size_of::<MpBusEntry>(),
            MP_BASETABLE_PROCESSOR_ENTRY => size_of::<MpProcessorEntry>(),
            MP_BASETABLE_IOINT_ASSIGN_ENTRY => size_of::<IoIntAssignEntry>(),
            MP_BASETABLE_LOCALINT_ASSIGN_ENTRY => size_of::<LocalIntAssignEntry>(),
            _ => panic!("unknown MP table entry type={}", entry_type),
        };

        entry_ptr += entry_size;
    }

    panic!("I/O APIC not found");
}
