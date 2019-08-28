#include <arch.h>
#include <printk.h>
#include <x64/apic.h>
#include <x64/smp.h>
#include <x64/x64.h>

static struct mp_float_ptr *look_for_floatptr_table(
    paddr_t start, paddr_t end) {
    vaddr_t end_vaddr = (vaddr_t) from_paddr(end);
    for (uint32_t *p = from_paddr(start); (vaddr_t) p < end_vaddr; p++) {
        if (*p == MP_FLOATPTR_SIGNATURE) {
            return (struct mp_float_ptr *) p;
        }
    }

    return NULL;
}

void x64_smp_init(void) {
    struct mp_table_header *mptblhdr;
    struct mp_ioapic_entry *ioapic_entry;
    void *entry_ptr;

    struct mp_float_ptr *mpfltptr = look_for_floatptr_table(0xf0000, 0x100000);
    if (mpfltptr == NULL) {
        PANIC("MP table not found");
        return;
    }

    mptblhdr = (struct mp_table_header *) from_paddr(
        (paddr_t) mpfltptr->mptable_header_addr);
    if (mptblhdr->signature != MP_MPTABLE_SIGNATURE) {
        PANIC("invalid MP table");
        return;
    }

    entry_ptr = (void *) ((paddr_t) mptblhdr + sizeof(struct mp_table_header));
    for (int i = 0; i < mptblhdr->entry_count; i++) {
        size_t size;
        uint8_t type = *((uint8_t *) entry_ptr);
        switch (type) {
        case MP_BASETABLE_IOAPIC_ENTRY:
            ioapic_entry = (struct mp_ioapic_entry *) entry_ptr;
            size = sizeof(struct mp_ioapic_entry);
            if (ioapic_entry->ioapic_flags != 0) {
                x64_ioapic_init(ioapic_entry->memmaped_ioapic_addr);
            }
            break;
        case MP_BASETABLE_BUS_ENTRY:
            size = sizeof(struct mp_bus_entry);
            break;
        case MP_BASETABLE_PROCESSOR_ENTRY:
            size = sizeof(struct mp_processor_entry);
            break;
        case MP_BASETABLE_IOINT_ASSIGN_ENTRY:
            size = sizeof(struct mp_ioint_assign_entry);
            break;
        case MP_BASETABLE_LOCALINT_ASSIGN_ENTRY:
            size = sizeof(struct mp_localint_assign_entry);
            break;
        default:
            PANIC("unknown mp table entry: %d", type);
        }

        entry_ptr = (void *) ((paddr_t) entry_ptr + (paddr_t) size);
    }
}
