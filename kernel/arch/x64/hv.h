#ifndef __X64_HV_H__
#define __X64_HV_H__

#include <types.h>

#define VMCS_PINBASED_CTLS                 0x4000
#define VMCS_PROCBASED_CTLS1               0x4002
#define VMCS_VM_EXIT_MSR_STORE_COUNT       0x400e
#define VMCS_VM_EXIT_MSR_LOAD_COUNT        0x4010
#define VMCS_VM_EXIT_CTLS                  0x400c
#define VMCS_VM_ENTRY_CTLS                 0x4012
#define VM_ENTRY_CTLS_LONG_MODE_GUEST      (1 << 9)
#define VMCS_VM_ENTRY_MSR_LOAD_COUNT       0x4014
#define VMCS_PROCBASED_CTLS2               0x401e
#define PROCBASED_CTLS2_EPT                (1u << 1)
#define PROCBASED_CTLS2_UNRESTRICTED_GUEST (1u << 7)
#define PROCBASED_CTLS2_XSAVE              (1u << 20)
#define VMCS_INST_ERROR                    0x4400
#define VMCS_VM_EXIT_REASON                0x4402
#define VMCS_VMEXIT_INSTRUCTION_LEN        0x440c
#define VMCS_VMEXIT_INSTRUCTION_INFO       0x440e
#define VMCS_GUEST_PHYSICAL_ADDR           0x2400
#define VMCS_GUEST_LINEAR_ADDR             0x640a
#define VMCS_VMEXIT_QUALIFICATION          0x6400

#define VMCS_VM_EXIT_MSR_STORE_ADDR 0x2006
#define VMCS_VM_EXIT_MSR_LOAD_ADDR  0x2008
#define VMCS_VM_ENTRY_MSR_LOAD_ADDR 0x200a
#define VMCS_EPT                    0x201a
#define EPT_4LEVEL                  (3 << 3)
#define EPT_TYPE_WB                 6
#define VMCS_LINK_POINTER           0x2800

#define VMCS_HOST_ES_SEL    0x0c00
#define VMCS_HOST_CS_SEL    0x0c02
#define VMCS_HOST_SS_SEL    0x0c04
#define VMCS_HOST_DS_SEL    0x0c06
#define VMCS_HOST_FS_SEL    0x0c08
#define VMCS_HOST_GS_SEL    0x0c0a
#define VMCS_HOST_TR_SEL    0x0c0c
#define VMCS_HOST_IA32_PAT  0x2c00
#define VMCS_HOST_IA32_EFER 0x2c02
#define VMCS_HOST_CR0       0x6c00
#define VMCS_HOST_CR3       0x6c02
#define VMCS_HOST_CR4       0x6c04
#define VMCS_HOST_GSBASE    0x6c08
#define VMCS_HOST_TRBASE    0x6c0a
#define VMCS_HOST_GDTR_BASE 0x6c0c
#define VMCS_HOST_IDTR_BASE 0x6c0e
#define VMCS_HOST_RIP       0x6c16
#define VMCS_HOST_RSP       0x6c14

#define VMCS_GUEST_ES_SEL                 0x0800
#define VMCS_GUEST_CS_SEL                 0x0802
#define VMCS_GUEST_SS_SEL                 0x0804
#define VMCS_GUEST_DS_SEL                 0x0806
#define VMCS_GUEST_FS_SEL                 0x0808
#define VMCS_GUEST_GS_SEL                 0x080a
#define VMCS_GUEST_LDTR_SEL               0x080c
#define VMCS_GUEST_TR_SEL                 0x080e
#define VMCS_GUEST_IA32_EFER              0x2806
#define VMCS_GUEST_CR0                    0x6800
#define VMCS_GUEST_CR3                    0x6802
#define VMCS_GUEST_CR4                    0x6804
#define VMCS_GUEST_ES_BASE                0x6806
#define VMCS_GUEST_CS_BASE                0x6808
#define VMCS_GUEST_SS_BASE                0x680a
#define VMCS_GUEST_DS_BASE                0x680c
#define VMCS_GUEST_FS_BASE                0x680e
#define VMCS_GUEST_GS_BASE                0x6810
#define VMCS_GUEST_TR_BASE                0x6814
#define VMCS_GUEST_GDTR_BASE              0x6816
#define VMCS_GUEST_IDTR_BASE              0x6818
#define VMCS_GUEST_RSP                    0x681c
#define VMCS_GUEST_RIP                    0x681e
#define VMCS_GUEST_RFLAGS                 0x6820
#define VMCS_GUEST_ES_LIMIT               0x4800
#define VMCS_GUEST_CS_LIMIT               0x4802
#define VMCS_GUEST_SS_LIMIT               0x4804
#define VMCS_GUEST_DS_LIMIT               0x4806
#define VMCS_GUEST_FS_LIMIT               0x4808
#define VMCS_GUEST_GS_LIMIT               0x480a
#define VMCS_GUEST_LDTR_LIMIT             0x480c
#define VMCS_GUEST_TR_LIMIT               0x480e
#define VMCS_GUEST_GDTR_LIMIT             0x4810
#define VMCS_GUEST_IDTR_LIMIT             0x4812
#define VMCS_GUEST_ES_ACCESS              0x4814
#define VMCS_GUEST_CS_ACCESS              0x4816
#define VMCS_GUEST_SS_ACCESS              0x4818
#define VMCS_GUEST_DS_ACCESS              0x481a
#define VMCS_GUEST_FS_ACCESS              0x481c
#define VMCS_GUEST_GS_ACCESS              0x481e
#define VMCS_GUEST_LDTR_ACCESS            0x4820
#define VMCS_GUEST_TR_ACCESS              0x4822
#define VMCS_GUEST_INTERRUPTIBILITY_STATE 0x4824
#define VMCS_GUEST_ACTIVITY_STATE         0x4826

#define VMCS_CR0_MASK        0x6000
#define VMCS_CR4_MASK        0x6002
#define VMCS_CR0_READ_SHADOW 0x6004
#define VMCS_CR4_READ_SHADOW 0x6006

#define VMCS_VMEXIT_INTR_INFO  0x4004
#define VMCS_VMENTRY_INTR_INFO 0x4016

#define MSR_IA32_VMX_BASIC                0x0480
#define MSR_IA32_VMX_PINBASED_CTLS        0x0481
#define VMX_PINBASED_CTLS_EXIT_BY_EXT_INT (1u << 0)
#define VMX_PINBASED_CTLS_EXIT_BY_NMI     (1u << 3)
#define MSR_IA32_VMX_PROCBASED_CTLS1      0x0482
#define VMX_PROCBASED_CTLS1_EXIT_BY_HLT   (1u << 7)
#define VMX_PROCBASED_CTLS1_EXIT_BY_IO    (1u << 24)
#define VMX_PROCBASED_CTLS1_ENABLE_CTLS2  (1u << 31)
#define MSR_IA32_VMX_VM_EXIT_CTLS         0x0483
#define VMX_VM_EXIT_CTLS_SAVE_DEBUG_CTLS  (1u << 2)
#define VMX_VM_EXIT_CTLS_HOST_IS_64BIT    (1u << 9)
#define VMX_VM_EXIT_CTLS_SAVE_IA32_EFER   (1u << 20)
#define VMX_VM_EXIT_CTLS_LOAD_IA32_EFER   (1u << 21)
#define MSR_IA32_VMX_VM_ENTRY_CTLS        0x0484
#define VMX_VM_ENTRY_CTLS_LOAD_DEBUG_CTLS (1u << 2)
#define VMX_VM_ENTRY_CTLS_LOAD_IA32_EFER  (1u << 15)
#define MSR_IA32_VMX_PROCBASED_CTLS2      0x048b
#define MSR_IA32_VMX_TRUE_PINBASED_CTLS   0x048d
#define MSR_IA32_VMX_TRUE_PROCBASED_CTLS  0x048e
#define MSR_PAT                           0x0277

#define MSR_FS_BASE 0xc0000100
#define MSR_GS_BASE 0xc0000101

#define VMEXIT_EXTERNAL_IRQ        1
#define VMEXIT_TRIPLE_FAULT        2
#define VMEXIT_CPUID               10
#define VMEXIT_HLT                 12
#define VMEXIT_CR_ACCESS           28
#define VMEXIT_IO_INSTRUCTION      30
#define VMEXIT_RDMSR               31
#define VMEXIT_WRMSR               32
#define VMEXIT_INVALID_GUEST_STATE 33
#define VMEXIT_EPT_VIOLATION       48
#define VMEXIT_XSETBV              55

#define EXITQ_CR_ACCESS_TYPE_MOV_TO_CR   0
#define EXITQ_CR_ACCESS_TYPE_MOV_FROM_CR 1
#define EXITQ_CR_ACCESS_TYPE_MOV_CLTS    2
#define EXITQ_CR_ACCESS_TYPE_MOV_LMSW    3

#define VMENTRY_INTR_TYPE_EXT (0 << 8)
#define VMENTRY_INTR_VALID    (1u << 31)

#define CR0_PG   (1u << 31)
#define EFER_LMA (1u << 10)
#define EFER_LME (1u << 8)

#define VMX_SERIAL_IRQ 4
#define PIC_ICW1_INIT  0x10

#define REG8(reg)  ((reg) &0xff)
#define REG32(reg) ((reg) &0xffffffff)

// Set lower 8-bit register. `reg` must be rax, rbx, ...
#define SET8(regs, reg, value)                                                 \
    do {                                                                       \
        regs->reg = ((regs->reg & ~0xffull) | value);                          \
    } while (0)

// Set lower 16-bit register. `reg` must be rax, rbx, ...
#define SET16(regs, reg, value)                                                \
    do {                                                                       \
        regs->reg = ((regs->reg & ~0xffffull) | value);                        \
    } while (0)

// Set lower 32-bit register and clears upper 32 bits. `reg` must be rax, rbx,
// ...
#define SET32(regs, reg, value)                                                \
    do {                                                                       \
        regs->reg = (value);                                                   \
    } while (0)

#define SETn(regs, reg, size, value)                                           \
    do {                                                                       \
        switch (size) {                                                        \
            case 1:                                                            \
                SET8(regs, reg, value);                                        \
                break;                                                         \
            case 2:                                                            \
                SET16(regs, reg, value);                                       \
                break;                                                         \
            case 4:                                                            \
                SET32(regs, reg, value);                                       \
                break;                                                         \
            default:                                                           \
                UNREACHABLE();                                                 \
        }                                                                      \
    } while (0)

#define ASSERT_VM_INST(expr)                                                   \
    do {                                                                       \
        error_t err = (expr);                                                  \
        switch (err) {                                                         \
            case ERR_INVALID_ARG:                                              \
                PANIC("%s failed (VMFailInvalid)", #expr);                     \
                break;                                                         \
            case ERR_ABORTED:                                                  \
                PANIC("%s failed (VM-instruction error code = %d)", #expr,     \
                      asm_vmread(VMCS_INST_ERROR));                            \
                break;                                                         \
        }                                                                      \
    } while (0)

#define HV_NYI(fmt, ...)                                                       \
    do {                                                                       \
        printf(SGR_WARN_DBG "[%s] NYI: " fmt SGR_RESET "\n", __program_name(), \
               ##__VA_ARGS__);                                                 \
        task_exit(EXP_HV_UNIMPLEMENTED);                                       \
    } while (0)

#define ASSERT_GUEST(expr)                                                     \
    do {                                                                       \
        if (!(expr)) {                                                         \
            WARN_DBG("[%s] %s:%d: GUEST ASSERTION FAILURE: %s", CURRENT->name, \
                     __FILE__, __LINE__, #expr);                               \
            WARN_DBG("terminating %s due to the assertion...", CURRENT->name); \
            task_exit(EXP_HV_UNIMPLEMENTED);                                   \
        }                                                                      \
    } while (0)

struct __packed vmcs {
    uint32_t revision;
    uint32_t aborted_by;
    char data[PAGE_SIZE - sizeof(uint32_t) * 2];
};

struct pic {
    int init_phase;
    uint8_t vector_base;
    uint8_t irq_mask;
};

#define CURRENT_VMX            (CURRENT->arch.vmx)
#define CURRENT_VMX_MASTER_PIC (CURRENT->arch.vmx.pics.master)
#define CURRENT_VMX_SLAVE_PIC  (CURRENT->arch.vmx.pics.slave)
struct vmx {
    struct vmcs *vmcs;
    struct saved_msrs *saved_msrs;
    uint32_t saved_cr2;
    bool long_mode;
    bool launched;
    uint32_t pci_addr;
    uint16_t pending_irq_bitmap;
    struct {
        struct pic master;
        struct pic slave;
    } pics;
};

/// The register values in the guest on VM exit.
struct __packed guest_regs {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
};

struct __packed saved_msr_entry {
    uint32_t index;
    uint32_t reserved;
    uint64_t value;
};

// The size of `host` must be a multiplies of PAGE_SIZE so that `guest` is
// aliged to PAGE_SIZE.
#define NUM_SAVED_MSRS_MAX (PAGE_SIZE / sizeof(struct saved_msr_entry))

struct saved_msrs {
    struct saved_msr_entry host[NUM_SAVED_MSRS_MAX];
    struct saved_msr_entry guest[NUM_SAVED_MSRS_MAX];
    union {
        uint8_t remaining[PAGE_SIZE];
        struct {
            size_t num_entries;
        };
    };
};

static inline void asm_vmwrite(uint64_t field, uint64_t value) {
    __asm__ __volatile__(
        "vmwrite %[value], %[field]" ::[value] "r"(value), [field] "r"(field));
}

static inline uint64_t asm_vmread(uint64_t field) {
    uint64_t value;
    __asm__ __volatile__("vmread %[field], %[value]"
                         : [value] "=r"(value)
                         : [field] "r"(field)
                         : "cc");
    return value;
}

static inline void asm_vmclear(paddr_t vmcs) {
    __asm__ __volatile__("vmclear %0" ::"m"(vmcs) : "cc");
}

static inline error_t asm_vmxon(paddr_t vmx_area) {
    uint8_t cf, zf;
    __asm__ __volatile__(
        "vmxon %[vmx_area];"
        "setc %[cf];"
        "setz %[zf];"
        : [cf] "=r"(cf), [zf] "=r"(zf)
        : [vmx_area] "m"(vmx_area)
        : "cc");

    if (cf) {
        return ERR_INVALID_ARG;
    }

    if (zf) {
        return ERR_ABORTED;
    }

    return OK;
}

static inline error_t asm_vmptrld(paddr_t vmcs) {
    uint8_t cf, zf;
    __asm__ __volatile__(
        "vmptrld %[vmcs];"
        "setc %[cf];"
        "setz %[zf];"
        : [cf] "=r"(cf), [zf] "=r"(zf)
        : [vmcs] "m"(vmcs)
        : "cc");

    if (cf) {
        return ERR_INVALID_ARG;
    }

    if (zf) {
        return ERR_ABORTED;
    }

    return OK;
}

static inline error_t asm_invept(uint64_t type, uint64_t ept_pml4) {
    uint8_t cf, zf;
    uint64_t invept_desc[2] = {ept_pml4, 0};
    __asm__ __volatile__(
        "invept %[desc], %[type];"
        "setc %[cf];"
        "setz %[zf];"
        : [cf] "=r"(cf), [zf] "=r"(zf)
        : [type] "r"(type), [desc] "m"(invept_desc)
        : "cc");

    if (cf) {
        return ERR_INVALID_ARG;
    }

    if (zf) {
        return ERR_ABORTED;
    }

    return OK;
}

static inline error_t asm_vmlaunch(struct guest_regs *initial_regs) {
    uint8_t cf, zf;
    __asm__ __volatile__(
        "mov %[regs], %%rsp   \n\t"
        "pop %%rax            \n\t"
        "pop %%rbx            \n\t"
        "pop %%rcx            \n\t"
        "pop %%rdx            \n\t"
        "pop %%rdi            \n\t"
        "pop %%rsi            \n\t"
        "pop %%rbp            \n\t"
        "pop %%r8             \n\t"
        "pop %%r9             \n\t"
        "pop %%r10            \n\t"
        "pop %%r11            \n\t"
        "pop %%r12            \n\t"
        "pop %%r13            \n\t"
        "pop %%r14            \n\t"
        "pop %%r15            \n\t"
        "vmlaunch             \n\t"
        "setc %[cf]           \n\t"
        "setz %[zf]           \n\t"
        : [zf] "=a"(zf), [cf] "=c"(cf)
        : [regs] "m"(initial_regs)
        : "%rbx", "%rdx", "%rdi", "%rsi", "%rbp", "%r8", "%r9", "%r10", "%r11",
          "%r12", "%r13", "%r14", "%r15", "cc");

    if (cf) {
        return ERR_INVALID_ARG;
    }

    if (zf) {
        return ERR_ABORTED;
    }

    return OK;
}

static inline error_t asm_vmresume(struct guest_regs *regs) {
    uint8_t cf, zf;
    __asm__ __volatile__(
        "mov %[regs], %%rsp   \n\t"
        "pop %%rax            \n\t"
        "pop %%rbx            \n\t"
        "pop %%rcx            \n\t"
        "pop %%rdx            \n\t"
        "pop %%rdi            \n\t"
        "pop %%rsi            \n\t"
        "pop %%rbp            \n\t"
        "pop %%r8             \n\t"
        "pop %%r9             \n\t"
        "pop %%r10            \n\t"
        "pop %%r11            \n\t"
        "pop %%r12            \n\t"
        "pop %%r13            \n\t"
        "pop %%r14            \n\t"
        "pop %%r15            \n\t"
        "vmresume             \n\t"
        "setc %[cf]           \n\t"
        "setz %[zf]           \n\t"
        : [zf] "=a"(zf), [cf] "=c"(cf)
        : [regs] "m"(regs)
        : "%rbx", "%rdx", "%rdi", "%rsi", "%rbp", "%r8", "%r9", "%r10", "%r11",
          "%r12", "%r13", "%r14", "%r15", "cc");

    if (cf) {
        return ERR_INVALID_ARG;
    }

    if (zf) {
        return ERR_ABORTED;
    }

    return OK;
}

// Defined in trap.S
void x64_vmexit_enty(void);

void x64_hv_start_guest(void);
void x64_hv_init(void);

#endif
