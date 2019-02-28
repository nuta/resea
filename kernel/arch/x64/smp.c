#include <memory.h>
#include <printk.h>
#include <x64/smp.h>
#include <x64/ioapic.h>

void x64_smp_init(void) {
    struct mp_float_ptr *mpfltptr = NULL;
    struct mp_table_header *mptblhdr;
    struct mp_ioapic_entry *ioapic_entry;
    void *entry_ptr;

    // look for MP Floating Pointer Table
    uint8_t *p = from_paddr(0xf0000);
    for (int i = 0x10000; i > 0; i--) {
        if (p[0]=='_' && p[1] == 'M' && p[2] == 'P' && p[3]=='_') {
            mpfltptr = (struct mp_float_ptr *) p;
            break;
        }

        p++;
    }

    if (mpfltptr == NULL) {
        INFO("MP table not found");
        return;
    }

    mptblhdr = (struct mp_table_header *) from_paddr((paddr_t) mpfltptr->mptable_header_addr);

    if (
        mptblhdr->signature[0] != 'P' || mptblhdr->signature[1] != 'C' ||
        mptblhdr->signature[2] != 'M' || mptblhdr->signature[3] != 'P'
    ) {
        INFO("invalid MP table");
        return;
    }

    entry_ptr = (void *) ((paddr_t) mptblhdr + sizeof(struct mp_table_header));
    for (int i=0; i < mptblhdr->entry_count; i++) {
        size_t size;
        switch(*((uint8_t *) entry_ptr)) {
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
                /* FIXME: Unknown entry. Abort for now. */
                return;
        }

        entry_ptr = (void *) ((paddr_t) entry_ptr + (paddr_t) size);
    }
}
