#include "mp.h"
#include <arch.h>
#include <printk.h>
#include <string.h>
#include <task.h>

// Note: these symbols points to **physical** addresses.
extern char boot_gdtr[];
extern char mpboot[];
extern char mpboot_end[];

static int num_cpus = 1;

int mp_num_cpus(void) {
    return num_cpus;
}

static void udelay(int usec) {
    while (usec-- > 0) {
        asm_in8(0x80);
    }
}

static void send_ipi(uint8_t vector, enum ipi_dest dest, uint8_t dest_cpu,
                     enum ipi_mode mode) {
    uint64_t data = ((uint64_t) dest_cpu << 56) | ((uint64_t) dest << 18)
                    | (1ULL << 14) | ((uint64_t) mode << 8) | vector;

    write_apic(APIC_REG_ICR_HIGH, data >> 32);
    write_apic(APIC_REG_ICR_LOW, data & 0xffffffff);
}

static struct mp_float_ptr *look_for_floatptr_table(paddr_t start,
                                                    paddr_t end) {
    vaddr_t end_vaddr = (vaddr_t) from_paddr(end);
    for (uint32_t *p = from_paddr(start); (vaddr_t) p < end_vaddr; p++) {
        if (*p == MP_FLOATPTR_SIGNATURE) {
            return (struct mp_float_ptr *) p;
        }
    }

    return NULL;
}

static void read_mp_table(void) {
    struct mp_table_header *mptblhdr;
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
                size = sizeof(struct mp_ioapic_entry);
                break;
            case MP_BASETABLE_BUS_ENTRY:
                size = sizeof(struct mp_bus_entry);
                break;
            case MP_BASETABLE_PROCESSOR_ENTRY:
                size = sizeof(struct mp_processor_entry);
                struct mp_processor_entry *entry = entry_ptr;
                if (entry->localapic_id != mp_cpuid()) {
                    num_cpus++;
                }
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

static void start_aps(void) {
    size_t mpboot_size = (size_t) mpboot_end - (size_t) mpboot;
    DEBUG_ASSERT(mpboot_size <= (paddr_t) __mp_boot_trampoine_end
                                    - (paddr_t) __mp_boot_trampoine);

    // Copy MP boot code into the lower address (addressable in real mode).
    memcpy(from_paddr((paddr_t) __mp_boot_trampoine),
           from_paddr((paddr_t) mpboot), mpboot_size);
    // Copy a GDTR into the lower address (addressable in real mode).
    memcpy(from_paddr((paddr_t) __mp_boot_gdtr),
           from_paddr((paddr_t) boot_gdtr), sizeof(struct gdtr));
    // Send IPI to APs to boot them.
    for (int cpu = 1; cpu < mp_num_cpus(); cpu++) {
        TRACE("starting CPU #%d...", cpu);
        send_ipi(0, IPI_DEST_UNICAST, cpu, IPI_MODE_INIT);
        udelay(20000);
        send_ipi(((paddr_t) __mp_boot_trampoine) >> 12, IPI_DEST_UNICAST, cpu,
                 IPI_MODE_STARTUP);
        udelay(4000);
    }
}

void mp_start(void) {
    read_mp_table();
    start_aps();
}

static int big_lock = UNLOCKED;
static int lock_owner = NO_LOCK_OWNER;

void lock(void) {
    if (mp_cpuid() == lock_owner) {
        PANIC("recusive lock (#%d)", mp_cpuid());
    }

    while (!__sync_bool_compare_and_swap(&big_lock, UNLOCKED, LOCKED)) {
        __asm__ __volatile__("pause");
    }

    lock_owner = mp_cpuid();
}

void unlock(void) {
    lock_owner = NO_LOCK_OWNER;
    __sync_bool_compare_and_swap(&big_lock, LOCKED, UNLOCKED);
}

void mp_reschedule(void) {
    send_ipi(VECTOR_IPI_RESCHEDULE, IPI_DEST_ALL_BUT_SELF, 0, IPI_MODE_FIXED);
}

void halt(void) {
    send_ipi(VECTOR_IPI_HALT, IPI_DEST_ALL_BUT_SELF, 0, IPI_MODE_FIXED);
    while (true) {
        __asm__ __volatile__("cli; hlt");
    }
}
