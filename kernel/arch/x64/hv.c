#include <task.h>
#include <ipc.h>
#include <printk.h>
#include "hv.h"
#include "arch.h"

static __aligned(PAGE_SIZE) uint8_t vmx_area[PAGE_SIZE];
static __aligned(PAGE_SIZE) struct vmcs vmcs_areas[CONFIG_NUM_TASKS];
static __aligned(PAGE_SIZE) struct saved_msrs saved_msrs[CONFIG_NUM_TASKS];

static uint32_t compute_ctrl_caps(uint32_t msr, uint32_t value) {
    // TODO: capability checks
    uint64_t caps = asm_rdmsr(msr);
    value |= caps & 0xffffffff;
    value &= caps >> 32;
    return value;
}

static uint64_t get_value_by_reg_index(struct guest_regs *regs, int index) {
    switch (index) {
        case 0: return regs->rax;
        case 1: return regs->rcx;
        case 2: return regs->rdx;
        case 3: return regs->rbx;
        case 4: return asm_vmread(VMCS_GUEST_RSP);
        case 5: return regs->rbp;
        case 6: return regs->rsi;
        case 7: return regs->rdi;
        case 8: return regs->r8;
        case 9: return regs->r9;
        case 10: return regs->r10;
        case 11: return regs->r11;
        case 12: return regs->r12;
        case 13: return regs->r13;
        case 14: return regs->r14;
        case 15: return regs->r15;
    }

    UNREACHABLE();
}

static void set_value_by_reg_index(struct guest_regs *regs, int index,
                                   uint64_t value) {
    switch (index) {
        case 0: regs->rax = value; return;
        case 1: regs->rcx = value; return;
        case 2: regs->rdx = value; return;
        case 3: regs->rbx = value; return;
        case 4: asm_vmwrite(VMCS_GUEST_RSP, value); return;
        case 5: regs->rbp = value; return;
        case 6: regs->rsi = value; return;
        case 7: regs->rdi = value; return;
        case 8: regs->r8 = value; return;
        case 9: regs->r9 = value; return;
        case 10: regs->r10 = value; return;
        case 11: regs->r11 = value; return;
        case 12: regs->r12 = value; return;
        case 13: regs->r13 = value; return;
        case 14: regs->r14 = value; return;
        case 15: regs->r15 = value; return;
    }

    UNREACHABLE();
}

static void activate_long_mode_if_needed(void) {
    if (CURRENT_VMX.long_mode) {
        return;
    }

    uint64_t cr0 = asm_vmread(VMCS_GUEST_CR0);
    uint64_t efer = asm_vmread(VMCS_GUEST_IA32_EFER);
    if ((efer & EFER_LME) != 0 && (cr0 & CR0_PG) != 0) {
        TRACE("hv: activating the long mode");
        asm_vmwrite(VMCS_GUEST_IA32_EFER, efer | EFER_LMA);
        asm_vmwrite(
            VMCS_VM_ENTRY_CTLS,
            asm_vmread(VMCS_VM_ENTRY_CTLS)
            | VM_ENTRY_CTLS_LONG_MODE_GUEST
        );
        CURRENT_VMX.long_mode = true;
    }
}

/// Looks for the saved MSR entry for the guest.
static struct saved_msr_entry *lookup_msr(uint32_t index) {
    struct saved_msrs *msrs = CURRENT_VMX.saved_msrs;
    for (size_t i = 0; i < msrs->num_entries; i++) {
        if (msrs->guest[i].index == index) {
            return &msrs->guest[i];
        }
    }

    return NULL;
}

/// Registers a MSR entry. It must be called before the guest reads the register.
static struct saved_msr_entry *register_msr(uint32_t index, uint64_t value) {
    struct saved_msrs *msrs = CURRENT_VMX.saved_msrs;
    ASSERT(msrs->num_entries < NUM_SAVED_MSRS_MAX && "too many saved MSRs");

    struct saved_msr_entry *new_guest_entry = &msrs->guest[msrs->num_entries];
    new_guest_entry->index = index;
    new_guest_entry->value = value;
    new_guest_entry->reserved = 0;

    // FIXME: RDMSR may cause #GP if `index` does not exist.
    struct saved_msr_entry *new_host_entry = &msrs->host[msrs->num_entries];
    new_host_entry->index = index;
    new_host_entry->value = asm_rdmsr(index);
    new_host_entry->reserved = 0;
    msrs->num_entries++;

    asm_vmwrite(VMCS_VM_EXIT_MSR_STORE_COUNT, msrs->num_entries);
    asm_vmwrite(VMCS_VM_EXIT_MSR_LOAD_COUNT, msrs->num_entries);
    asm_vmwrite(VMCS_VM_ENTRY_MSR_LOAD_COUNT, msrs->num_entries);
    return new_guest_entry;
}

void handle_rdmsr(uint32_t index, uint32_t *eax, uint32_t *edx) {
    uint64_t  value;
    switch (index) {
        case MSR_EFER: {
            value = asm_vmread(VMCS_GUEST_IA32_EFER);
            break;
        }
        case MSR_GS_BASE:
            value = asm_vmread(VMCS_GUEST_GS_BASE);
            break;
        case MSR_FS_BASE:
            value = asm_vmread(VMCS_GUEST_FS_BASE);
            break;
        case MSR_APIC_BASE:
            value = 0xfee00000;
            break;
        default: {
            struct saved_msr_entry *msr = lookup_msr(index);
            if (!msr) {
                HV_NYI("uninitialized MSR access (index=%p)", index);
            }

            value = msr->value;
            break;
        }
    }

    *eax = value;
    *edx = value >> 32;
}

void handle_wrmsr(uint32_t index, uint64_t value) {
    switch (index) {
        case MSR_EFER:
            asm_vmwrite(VMCS_GUEST_IA32_EFER, value);
            activate_long_mode_if_needed();
            break;
        case MSR_GS_BASE:
            asm_vmwrite(VMCS_GUEST_GS_BASE, value);
            break;
        case MSR_FS_BASE:
            asm_vmwrite(VMCS_GUEST_FS_BASE, value);
            break;
        case MSR_APIC_BASE:
            HV_NYI("wrmsr APIC_BASE");
            break;
        default: {
            struct saved_msr_entry *msr = lookup_msr(index);
            if (msr) {
                msr->value = value;
            } else {
                // This is the first time to access the MSR. Register it in the
                // table.
                register_msr(index, value);
            }
        }
    }
}

void handle_cpuid(struct guest_regs *regs) {
    switch (REG32(regs->rax)) {
        // Basic CPUID Information
        case 0:
            // Maximum Input Value for Basic CPUID Information.
            regs->rax = 0x10;
            // CPU vendor (e.g. "Resea HV!!!!").
            regs->rbx = 0x65736552;
            regs->rcx = 0x21212121;
            regs->rdx = 0x56482061;
            break;
        case 0x1:
            // Version Information.
            regs->rax = 0;
            // Feature information:
            regs->rbx = 0;
            regs->rcx =
                (1u << 21) /* x2APIC */ |(1u << 26) /* XSAVE */
                | (1u << 19) /* SSE4.1 */ | (1u << 20) /* SSE4.2 */;
            regs->rdx =
                (1u << 0) /* FPU */ | (1u << 6) /* PAE */ | (1u << 9) /* APIC */;
            break;
        case 0x6:
            regs->rax = 0;
            regs->rbx = 0;
            regs->rcx = 0;
            regs->rdx = 0;
            break;
        case 0x7:
            regs->rax = 0;
            regs->rbx = (1u << 0) /* FSGSBASE */;
            regs->rcx = 0;
            regs->rdx = 0;
            break;
        case 0xd:
            // Supported XCR0 states.
            regs->rax = 0b111 /* x86, SSE, AVX */;
            regs->rbx = 0;
            regs->rcx = 0;
            regs->rdx = 0;
            break;
        case 0xf:
            regs->rax = 0;
            regs->rbx = 0;
            regs->rcx = 0;
            regs->rdx = 0;
            break;
        case 0x10:
            regs->rax = 0;
            regs->rbx = 0;
            regs->rcx = 0;
            regs->rdx = 0;
            break;
        case 0x80000000:
            regs->rax = 0x10;
            regs->rbx = 0;
            regs->rcx = 0;
            regs->rdx = 0;
        case 0x80000001:
            // Maximum Input Value for Extended Function CPUID Information.
            regs->rax = 0x6;
            regs->rbx = (1u << 11) /* SYSCALL/SYSRET */ | (1u << 29) /* Intel 64 */;
            regs->rcx = 0;
            regs->rdx = 0;
            break;
        // Invalid leaves. Used by Linux Kernel's Xen support.
        case 0x40000000 ... 0x40010000:
            regs->rax = 0;
            regs->rbx = 0;
            regs->rcx = 0;
            regs->rdx = 0;
            break;
        default:
            HV_NYI("unimplemented CPUID leaf %p", REG32(regs->rax));
            break;
    }
}

void call_pager(struct message *m, int expected_reply) {
    error_t err = ipc(CURRENT->pager, CURRENT->pager->tid,
                      (__user struct message *) m, IPC_CALL | IPC_KERNEL);
    if (IS_ERROR(err)) {
        WARN_DBG("%s: aborted kernel ipc", CURRENT->name);
        task_exit(EXP_ABORTED_KERNEL_IPC);
    }

    // Check if the reply is valid.
    if (expected_reply > 0 && m->type != expected_reply) {
        WARN_DBG("%s: invalid reply from pager (expected=%s, actual=%s)",
                 CURRENT->name, msgtype2str(expected_reply), msgtype2str(m->type));
        task_exit(EXP_INVALID_MSG_FROM_PAGER);
    }
}

void handle_ept_violation(struct guest_regs *regs, uint64_t guest_rip,
                          gpaddr_t gpaddr, size_t inst_len) {
    hv_frame_t frame;
    frame.rip = guest_rip;
    frame.inst_len = inst_len;
    frame.cr3 = asm_vmread(VMCS_GUEST_CR3);
    frame.rax = regs->rax;
    frame.rbx = regs->rbx;
    frame.rcx = regs->rcx;
    frame.rdx = regs->rdx;
    frame.rdi = regs->rdi;
    frame.rsi = regs->rsi;
    frame.rbp = regs->rbp;
    frame.r8 = regs->r8;
    frame.r9 = regs->r9;
    frame.r10 = regs->r10;
    frame.r11 = regs->r11;
    frame.r12 = regs->r12;
    frame.r13 = regs->r13;
    frame.r14 = regs->r14;
    frame.r15 = regs->r15;

    struct message m;
    m.type = HV_GUEST_PAGE_FAULT_MSG;
    m.hv_guest_page_fault.task = CURRENT->tid;
    m.hv_guest_page_fault.gpaddr = gpaddr;
    memcpy(&m.hv_guest_page_fault.frame, &frame, sizeof(frame));
    call_pager(&m, HV_GUEST_PAGE_FAULT_REPLY_MSG);

    asm_vmwrite(VMCS_GUEST_RIP, m.hv_guest_page_fault_reply.frame.rip);
    regs->rax = m.hv_guest_page_fault_reply.frame.rax;
    regs->rbx = m.hv_guest_page_fault_reply.frame.rbx;
    regs->rcx = m.hv_guest_page_fault_reply.frame.rcx;
    regs->rdx = m.hv_guest_page_fault_reply.frame.rdx;
    regs->rdi = m.hv_guest_page_fault_reply.frame.rdi;
    regs->rsi = m.hv_guest_page_fault_reply.frame.rsi;
    regs->rbp = m.hv_guest_page_fault_reply.frame.rbp;
    regs->r8 = m.hv_guest_page_fault_reply.frame.r8;
    regs->r9 = m.hv_guest_page_fault_reply.frame.r9;
    regs->r10 = m.hv_guest_page_fault_reply.frame.r10;
    regs->r11 = m.hv_guest_page_fault_reply.frame.r11;
    regs->r12 = m.hv_guest_page_fault_reply.frame.r12;
    regs->r13 = m.hv_guest_page_fault_reply.frame.r13;
    regs->r14 = m.hv_guest_page_fault_reply.frame.r14;
    regs->r15 = m.hv_guest_page_fault_reply.frame.r15;

    uint64_t ept_pml4 = asm_vmread(VMCS_EPT) & 0x0000ffffffff0000;
    ASSERT_VM_INST(asm_invept(1, ept_pml4));
}

void handle_cr_access(struct guest_regs *regs, int cr, int access_type, int reg) {
    switch (cr) {
        case 0: {
            switch (access_type) {
                case EXITQ_CR_ACCESS_TYPE_MOV_TO_CR: {
                    uint64_t value = get_value_by_reg_index(regs, reg);
                    // Linux's nested vmx implementation requires
                    // CR0.NX = 1.
                    asm_vmwrite(VMCS_GUEST_CR0, value | CR0_NX);
                    if ((value & CR0_PG) != 0) {
                        activate_long_mode_if_needed();
                    }
                    break;
                }
                default:
                    HV_NYI("CR%d access (access_type=%d)", cr, access_type);
            }
            break;
        }
        case 3: {
            switch (access_type) {
                case EXITQ_CR_ACCESS_TYPE_MOV_TO_CR: {
                    uint64_t value = get_value_by_reg_index(regs, reg);
                    asm_vmwrite(VMCS_GUEST_CR3, value);
                    break;
                }
                case EXITQ_CR_ACCESS_TYPE_MOV_FROM_CR: {
                    uint64_t value = asm_vmread(VMCS_GUEST_CR3);
                    set_value_by_reg_index(regs, reg, value);
                    break;
                }
                default:
                    HV_NYI("CR%d access (access_type=%d)", cr, access_type);
            }
            break;
        }
        case 4: {
            switch (access_type) {
                case EXITQ_CR_ACCESS_TYPE_MOV_TO_CR: {
                    // FIXME: Should we update read shadow as well?
                    uint64_t value = get_value_by_reg_index(regs, reg);
                    asm_vmwrite(VMCS_GUEST_CR4, value | CR4_VMXE);
                    break;
                }
                default:
                    HV_NYI("CR%d access (access_type=%d)", cr, access_type);
            }
            break;
        }
        default:
            HV_NYI("CR%d access", cr);
    }
}

void serial_write(char ch) {
    static char buf[512];
    static int offset = 0;
    if (ch != '\r') {
        buf[offset++] = ch;
        if (ch == '\n' || offset == sizeof(buf) - 1) {
            if (ch == '\n') {
                // Remove the newline from the buffer since
                // INFO() prepends a newline.
                offset--;
            }

            buf[offset] = '\0';
            INFO("[hv:%s] %s", CURRENT->name, buf);
            offset = 0;
        }
    }

    CURRENT_VMX.pending_irq_bitmap |= 1 << VMX_SERIAL_IRQ;
}

void handle_io(struct guest_regs *regs, uint16_t port, bool out, int access_size,
               bool is_string) {
    uint32_t value = regs->rax & 0xffffffff;

    // TRACE("VMExit by IO Instruction: port=%x, size=%d, value=%x, direct=%s",
    //       port, access_size, value, out ? "out" : "in");

    ASSERT(!is_string && "string io instructions are not supported");

    switch (port) {
        // 8042 PS/2 Controller.
        case 0x64:
            if (!out) {
                SETn(regs, rax, access_size, 0);
            }
            break;
        // PIC master.
        case 0x20:
            if ((value & PIC_ICW1_INIT) != 0) {
                CURRENT_VMX_MASTER_PIC.init_phase = 1;
            }
            break;
        case 0x21:
            if (out) {
                switch (CURRENT_VMX_MASTER_PIC.init_phase) {
                    case 0:
                        // Not in the initialization.
                        CURRENT_VMX_MASTER_PIC.irq_mask = REG8(value);
                        break;
                    case 1:
                        // ICW2
                        CURRENT_VMX_MASTER_PIC.vector_base = REG8(value);
                        CURRENT_VMX_MASTER_PIC.init_phase++;
                        break;
                    case 2:
                        // ICW3
                        CURRENT_VMX_MASTER_PIC.init_phase++;
                        break;
                    case 3:
                        // ICW4
                       CURRENT_VMX_MASTER_PIC.init_phase = 0;
                       break;
                    default:
                        UNREACHABLE();
                }
            } else {
                SETn(regs, rax, access_size, CURRENT_VMX_MASTER_PIC.irq_mask);
            }
            break;
        // PIC slave.
        case 0xa0:
            if ((value & PIC_ICW1_INIT) != 0) {
                CURRENT_VMX_SLAVE_PIC.init_phase = 1;
            }
            break;
        case 0xa1:
            if (out) {
                switch (CURRENT_VMX_SLAVE_PIC.init_phase) {
                    case 0:
                        // Not in the initialization.
                        CURRENT_VMX_SLAVE_PIC.irq_mask = REG8(value);
                        break;
                    case 1:
                        // ICW2
                        CURRENT_VMX_SLAVE_PIC.vector_base = REG8(value);
                        CURRENT_VMX_SLAVE_PIC.init_phase++;
                        break;
                    case 2:
                        // ICW3
                        CURRENT_VMX_SLAVE_PIC.init_phase++;
                        break;
                    case 3:
                        // ICW4
                       CURRENT_VMX_SLAVE_PIC.init_phase = 0;
                       break;
                    default:
                        UNREACHABLE();
                }
            } else {
                SETn(regs, rax, access_size, CURRENT_VMX_SLAVE_PIC.irq_mask);
            }
            break;
        // Serial port: Data.
        case 0x3f8:
            if (out) {
                serial_write(REG8(value));
            }
            break;
        // Serial port: Interrupt Enable Register.
        case 0x3f9:
            if (out) {
                if ((value & (1 << 1)) != 0) {
                    // Inject a serial IRQ.
                    CURRENT_VMX.pending_irq_bitmap |= 1 << VMX_SERIAL_IRQ;
                }
            }
            break;
        // Serial port: unsupported controls.
        case 0x3fa ... 0x3fc:
            break;
        // Serial port: Line Status Register.
        case 0x3fd:
            if (!out && access_size == 1) {
                SET8(regs, rax, 0x60); // TX ready
            }
            break;
        // Serial port: Modem Status Register.
        case 0x3fe:
            if (!out && access_size == 1) {
                SET8(regs, rax, 0xb0);
            }
            break;
        // Unsupported io ports.
        default: {
            if (out) {
                struct message m;
                m.type = HV_IOPORT_WRITE_MSG;
                m.hv_ioport_write.task = CURRENT->tid;
                m.hv_ioport_write.port = port;
                m.hv_ioport_write.size = access_size;
                m.hv_ioport_write.value = value;
                call_pager(&m, HV_IOPORT_WRITE_REPLY_MSG);
            } else {
                struct message m;
                m.type = HV_IOPORT_READ_MSG;
                m.hv_ioport_read.task = CURRENT->tid;
                m.hv_ioport_read.port = port;
                m.hv_ioport_read.size = access_size;
                call_pager(&m, HV_IOPORT_READ_REPLY_MSG);
                SETn(regs, rax, access_size, m.hv_ioport_read_reply.value);
            }
        }
    }
}

/// Populates CPU-local host states.
static void vmwrite_cpu_locals(void) {
    asm_vmwrite(VMCS_HOST_CR3, asm_read_cr3());
    asm_vmwrite(VMCS_HOST_GSBASE, asm_rdgsbase());
    asm_vmwrite(VMCS_HOST_RSP, CURRENT->arch.interrupt_stack);
    asm_vmwrite(VMCS_HOST_GDTR_BASE, (uint64_t) &ARCH_CPUVAR->gdtr);
    asm_vmwrite(VMCS_HOST_IDTR_BASE, (uint64_t) &ARCH_CPUVAR->idtr);
    asm_vmwrite(VMCS_HOST_TRBASE, (uint64_t) &ARCH_CPUVAR->tss);
}

static bool pic_is_irq_masked(int irq) {
    if (irq >= 8) {
        bool slave_masked = (CURRENT_VMX_MASTER_PIC.irq_mask & (1 << 2)) != 0;
        bool irq_masked = (CURRENT_VMX_SLAVE_PIC.irq_mask & (1 << (irq - 8))) != 0;
        return slave_masked || irq_masked;
    } else {
        return (CURRENT_VMX_MASTER_PIC.irq_mask & (1 << irq)) != 0;
    }
}

static bool inject_event_if_exists(void) {
    // Resumed this guest task. Continue executing...
    struct message m;
    error_t err = ipc(CURRENT->pager, IPC_ANY,
                        (__user struct message *) &m,
                        IPC_RECV | IPC_NOBLOCK | IPC_KERNEL);
    if (err == OK && m.type == NOTIFICATIONS_MSG) {
        if (m.notifications.data & NOTIFY_ASYNC) {
            m.type = HV_AWAIT_MSG;
            m.hv_await.task = CURRENT->tid;
            call_pager(&m, -1);
            switch (m.type) {
                case HV_INJECT_IRQ_MSG:
                    CURRENT_VMX.pending_irq_bitmap |= m.hv_inject_irq.irq_bitmap;
                    break;
                default:
                    WARN_DBG("hv: unknown async message (type=%d)", m.type);
            }
        }
    }

    bool interrupt_enabled = (asm_vmread(VMCS_GUEST_RFLAGS) & 0x200) != 0;
    if (interrupt_enabled) {
        // Interrupts are enabled. Look for the pending interrupts and inject it
        // into the guest if it is not masked in PIC.
        int vector = -1;
        static int next_irq = 0;
        int irq = next_irq;
        while (true) {
            bool is_pending =
                (CURRENT_VMX.pending_irq_bitmap & (1u << irq)) != 0;
            if (!pic_is_irq_masked(irq) && is_pending) {
                if (irq < 8) {
                    vector = CURRENT_VMX_MASTER_PIC.vector_base + irq;
                } else {
                    vector = CURRENT_VMX_SLAVE_PIC.vector_base + (irq - 8);
                }

                CURRENT_VMX.pending_irq_bitmap &= ~(1u << irq);
                next_irq = (irq >= 15) ? 0 : irq + 1;
                irq = 0;
                break;
            }

            irq = (irq >= 15) ? 0 : irq + 1;
            if (irq == next_irq) {
                break;
            }
        }

        if (vector >= 0) {
            asm_vmwrite(VMCS_VMENTRY_INTR_INFO,
                        VMENTRY_INTR_VALID | VMENTRY_INTR_TYPE_EXT | vector);
            return true;
        }

        // TODO: Should we inject other pending IRQs once the RFLAGS.IF
        //       is set?
    }

    return false;
}

void x64_handle_vmexit(struct guest_regs *regs) {
    lock();
    uint32_t exit_info = asm_vmread(VMCS_VM_EXIT_REASON);
    uint64_t exit_qual = asm_vmread(VMCS_VMEXIT_QUALIFICATION);
    const uint64_t guest_rip = asm_vmread(VMCS_GUEST_RIP);
    const uint8_t inst_len = asm_vmread(VMCS_VMEXIT_INSTRUCTION_LEN);
    uint16_t reason = exit_info & 0xffff;

    // TRACE("VMExit: exit_info=%p (reason=%d), exit_qual=%p, guest_rip=%p",
    //       exit_info, reason, exit_qual, guest_rip);

    // FIXME: Why?
    asm_lgdt((uint64_t) &ARCH_CPUVAR->gdtr);
    asm_lidt((uint64_t) &ARCH_CPUVAR->idtr);

    bool advance_rip = true;
    switch (reason) {
        case VMEXIT_EXTERNAL_IRQ: {
            advance_rip = false;
            // Handle the IRQ in the interrupt handler.
            unlock();
            __asm__ __volatile__("sti; nop; cli");
            lock();
            inject_event_if_exists();
            break;
        }
        case VMEXIT_EPT_VIOLATION: {
            advance_rip = false;
            paddr_t gpaddr = asm_vmread(VMCS_GUEST_PHYSICAL_ADDR);
            // uint64_t gvaddr = asm_vmread(VMCS_GUEST_LINEAR_ADDR);
            // TRACE("VMExit by EPT Violation: gpaddr=%p, gvaddr=%p", gpaddr, gvaddr);
            handle_ept_violation(regs, guest_rip, gpaddr, inst_len);
            break;
        }
        case VMEXIT_IO_INSTRUCTION: {
            uint16_t port = (exit_qual >> 16) & 0xffff;
            bool out = (exit_qual & (1 << 3)) == 0;
            bool is_string = (exit_qual & (1 << 4)) != 0;

            // The size of io access (i.e. outb, outw, outl).
            // 0 = 1-byte, 1 = 2-byte, 3 = 4-byte. Thus, add 1.
            int access_size = (exit_qual & 0x3) + 1;

            handle_io(regs, port, out, access_size, is_string);
            break;
        }
        case VMEXIT_CR_ACCESS: {
            int cr = exit_qual & 0b111;
            int access_type = (exit_qual >> 4) & 0b11;
            int reg = (exit_qual >> 8) & 0b1111;
            // TRACE("VMExit by CR Access: CR=%d, access_type=%d", cr, access_type);
            handle_cr_access(regs, cr, access_type, reg);
            break;
        }
        case VMEXIT_RDMSR: {
            uint32_t ecx = REG32(regs->rcx);
            // TRACE("VMExit by RDMSR: rcx=%p", ecx);

            uint32_t eax, edx;
            handle_rdmsr(ecx, &eax, &edx);
            regs->rax = eax;
            regs->rdx = edx;
            break;
        }
        case VMEXIT_WRMSR: {
            uint32_t ecx = REG32(regs->rcx);
            uint64_t value = (REG32(regs->rdx) << 32) | REG32(regs->rax);
            // TRACE("VMExit by WRMSR: rcx=%p, edx:eax=%p", ecx, value);
            handle_wrmsr(ecx, value);
            break;
        }
        case VMEXIT_CPUID: {
            handle_cpuid(regs);
            break;
        }
        case VMEXIT_XSETBV: {
            WARN_DBG("XSETBV is not yet supported, ignoring");
            break;
        }
        case VMEXIT_HLT:
            while (!inject_event_if_exists()) {
                struct message m;
                m.type = HV_HALT_MSG;
                m.hv_halt.task = CURRENT->tid;
                call_pager(&m, HV_HALT_REPLY_MSG);
            }

            asm_vmwrite(VMCS_GUEST_ACTIVITY_STATE, 0 /* ACTIVE */);
            asm_vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, 0);
            break;
        case VMEXIT_TRIPLE_FAULT:
            TRACE("VMExit by Triple Fault: guest_rip=%p", guest_rip);
            task_exit(EXP_HV_CRASHED);
            break;
        case VMEXIT_INVALID_GUEST_STATE:
            WARN_DBG("invalid guest state (name=%s)", CURRENT->name);
            task_exit(EXP_HV_INVALID_STATE);
            break;
        default:
            WARN_DBG("unsupported vmexit reason=%d (guest_rip=%p)",
                     reason, guest_rip);
            task_exit(EXP_HV_UNIMPLEMENTED);
    }

    // Reload the current task's VMCS since it may be changed during context
    // swithes occurred in IPC operations above.
    paddr_t vmcs_paddr = into_paddr(CURRENT_VMX.vmcs);
    ASSERT_VM_INST(asm_vmptrld(vmcs_paddr));

    // Rewrite CPU-local host stastes in case the current CPU is different
    // from the last execution.
    vmwrite_cpu_locals();

    // Skip the current instruction which occurred the VM exit.
    if (advance_rip) {
        asm_vmwrite(VMCS_GUEST_RIP, guest_rip + inst_len);
    }

    // Restore the guest's register values and then resume its execution.
    unlock();
    ASSERT_VM_INST(asm_vmresume(regs));
    UNREACHABLE();
}

static void init_vmcs(struct vmcs *vmcs, struct saved_msrs *msrs,
                      uint64_t guest_rip, paddr_t ept_pml4) {
    // Initialize VMCS.
    bzero(vmcs, sizeof(*vmcs));
    paddr_t vmcs_paddr = into_paddr(vmcs);
    vmcs->revision = asm_rdmsr(MSR_IA32_VMX_BASIC);
    asm_vmclear(vmcs_paddr);
    ASSERT_VM_INST(asm_vmptrld(vmcs_paddr));

    // Populate control flags.
    asm_vmwrite(VMCS_PINBASED_CTLS,
        compute_ctrl_caps(MSR_IA32_VMX_PINBASED_CTLS,
                          VMX_PINBASED_CTLS_EXIT_BY_EXT_INT
                          | VMX_PINBASED_CTLS_EXIT_BY_NMI));
    asm_vmwrite(VMCS_PROCBASED_CTLS1,
        compute_ctrl_caps(MSR_IA32_VMX_PROCBASED_CTLS1,
                          VMX_PROCBASED_CTLS1_EXIT_BY_HLT
                          | VMX_PROCBASED_CTLS1_EXIT_BY_IO
                          | VMX_PROCBASED_CTLS1_ENABLE_CTLS2));
    asm_vmwrite(VMCS_PROCBASED_CTLS2,
        compute_ctrl_caps(MSR_IA32_VMX_PROCBASED_CTLS2,
            PROCBASED_CTLS2_UNRESTRICTED_GUEST
            | PROCBASED_CTLS2_EPT
            | PROCBASED_CTLS2_XSAVE
        )
    );
    asm_vmwrite(VMCS_VM_ENTRY_CTLS,
        compute_ctrl_caps(MSR_IA32_VMX_VM_ENTRY_CTLS,
                          VMX_VM_ENTRY_CTLS_LOAD_DEBUG_CTLS
                          | VMX_VM_ENTRY_CTLS_LOAD_IA32_EFER));
    asm_vmwrite(VMCS_VM_EXIT_CTLS,
        compute_ctrl_caps(MSR_IA32_VMX_VM_EXIT_CTLS,
                          VMX_VM_EXIT_CTLS_HOST_IS_64BIT
                          | VMX_VM_EXIT_CTLS_SAVE_DEBUG_CTLS
                          | VMX_VM_EXIT_CTLS_SAVE_IA32_EFER
                          | VMX_VM_EXIT_CTLS_LOAD_IA32_EFER));

    asm_vmwrite(VMCS_EPT, ept_pml4 | EPT_4LEVEL | EPT_TYPE_WB);
    asm_vmwrite(VMCS_LINK_POINTER, 0xffffffffffffffff);

    asm_vmwrite(VMCS_VM_EXIT_MSR_STORE_ADDR, into_paddr(&msrs->guest));
    asm_vmwrite(VMCS_VM_EXIT_MSR_LOAD_ADDR, into_paddr(&msrs->host));
    asm_vmwrite(VMCS_VM_ENTRY_MSR_LOAD_ADDR, into_paddr(&msrs->guest));
    asm_vmwrite(VMCS_VM_EXIT_MSR_STORE_COUNT, msrs->num_entries);
    asm_vmwrite(VMCS_VM_EXIT_MSR_LOAD_COUNT, msrs->num_entries);
    asm_vmwrite(VMCS_VM_ENTRY_MSR_LOAD_COUNT, msrs->num_entries);

    // Don't cause VM-exit on CR0 reads/writes (except CR0.PG/CR4.VMXE changes).
    asm_vmwrite(VMCS_CR0_MASK, CR0_PG);
    asm_vmwrite(VMCS_CR4_MASK, CR4_VMXE);
    asm_vmwrite(VMCS_CR0_READ_SHADOW, 0);
    asm_vmwrite(VMCS_CR4_READ_SHADOW, 0);

    // Populate host states.
    asm_vmwrite(VMCS_HOST_CR0, asm_read_cr0());
    asm_vmwrite(VMCS_HOST_CR4, asm_read_cr4());
    asm_vmwrite(VMCS_HOST_CS_SEL, KERNEL_CS);
    asm_vmwrite(VMCS_HOST_DS_SEL, 0);
    asm_vmwrite(VMCS_HOST_ES_SEL, 0);
    asm_vmwrite(VMCS_HOST_FS_SEL, 0);
    asm_vmwrite(VMCS_HOST_GS_SEL, 0);
    asm_vmwrite(VMCS_HOST_SS_SEL, 0);
    asm_vmwrite(VMCS_HOST_TR_SEL, TSS_SEG);
    asm_vmwrite(VMCS_HOST_RIP, (vaddr_t) x64_vmexit_enty);
    asm_vmwrite(VMCS_HOST_IA32_EFER, asm_rdmsr(MSR_EFER));
    asm_vmwrite(VMCS_HOST_IA32_PAT, asm_rdmsr(MSR_PAT));

    // Populate guest initial states.
    asm_vmwrite(VMCS_GUEST_RIP, guest_rip);
    asm_vmwrite(VMCS_GUEST_RSP, 0);
    asm_vmwrite(VMCS_GUEST_RFLAGS, 0x2);
    asm_vmwrite(VMCS_GUEST_CR0,
        (1ul << 0) /* Protected Mode */
        | (1ul << 5) /* NX: required by Linux's nested VMX implementation */);
    asm_vmwrite(VMCS_GUEST_CR3, 0);
    asm_vmwrite(VMCS_GUEST_CR4, 1 << 13 /* VMXE */);
    asm_vmwrite(VMCS_GUEST_IA32_EFER, 0);
    asm_vmwrite(VMCS_GUEST_GDTR_BASE, 0);
    asm_vmwrite(VMCS_GUEST_IDTR_BASE, 0);
    asm_vmwrite(VMCS_GUEST_CS_SEL, 8);
    asm_vmwrite(VMCS_GUEST_SS_SEL, 16);
    asm_vmwrite(VMCS_GUEST_DS_SEL, 16);
    asm_vmwrite(VMCS_GUEST_ES_SEL, 16);
    asm_vmwrite(VMCS_GUEST_FS_SEL, 16);
    asm_vmwrite(VMCS_GUEST_GS_SEL, 16);
    asm_vmwrite(VMCS_GUEST_TR_SEL, 0);
    asm_vmwrite(VMCS_GUEST_LDTR_SEL, 0);
    asm_vmwrite(VMCS_GUEST_CS_BASE, 0x00000000);
    asm_vmwrite(VMCS_GUEST_SS_BASE, 0x00000000);
    asm_vmwrite(VMCS_GUEST_DS_BASE, 0x00000000);
    asm_vmwrite(VMCS_GUEST_ES_BASE, 0x00000000);
    asm_vmwrite(VMCS_GUEST_FS_BASE, 0x00000000);
    asm_vmwrite(VMCS_GUEST_GS_BASE, 0x00000000);
    asm_vmwrite(VMCS_GUEST_TR_BASE, 0x00000000);
    asm_vmwrite(VMCS_GUEST_CS_LIMIT, 0xffffffff);
    asm_vmwrite(VMCS_GUEST_SS_LIMIT, 0xffffffff);
    asm_vmwrite(VMCS_GUEST_DS_LIMIT, 0xffffffff);
    asm_vmwrite(VMCS_GUEST_ES_LIMIT, 0xffffffff);
    asm_vmwrite(VMCS_GUEST_FS_LIMIT, 0xffffffff);
    asm_vmwrite(VMCS_GUEST_GS_LIMIT, 0xffffffff);
    asm_vmwrite(VMCS_GUEST_TR_LIMIT, 0xffff);
    asm_vmwrite(VMCS_GUEST_LDTR_LIMIT, 0xffff);
    asm_vmwrite(VMCS_GUEST_GDTR_LIMIT, 0);
    asm_vmwrite(VMCS_GUEST_IDTR_LIMIT, 0x3ff);
    asm_vmwrite(VMCS_GUEST_CS_ACCESS, 0xc09b);
    asm_vmwrite(VMCS_GUEST_SS_ACCESS, 0xc093);
    asm_vmwrite(VMCS_GUEST_DS_ACCESS, 0xc093);
    asm_vmwrite(VMCS_GUEST_ES_ACCESS, 0xc093);
    asm_vmwrite(VMCS_GUEST_FS_ACCESS, 0xc093);
    asm_vmwrite(VMCS_GUEST_GS_ACCESS, 0xc093);
    asm_vmwrite(VMCS_GUEST_TR_ACCESS, 0x008b);
    asm_vmwrite(VMCS_GUEST_LDTR_ACCESS, 0x0082);
}

static void init_msrs(void) {
    register_msr(MSR_KERNEL_GS_BASE, 0);
}

void x64_hv_start_guest(void) {
    INFO("Starting a hv guest: name=%s", CURRENT->name);

    // TODO: Abort if vmx is not supported.

    struct message m;
    bzero(&m, sizeof(m));
    m.type = HV_X64_START_MSG;
    m.hv_x64_start.task = CURRENT->tid;
    call_pager(&m, HV_X64_START_REPLY_MSG);

    uint64_t guest_rip = m.hv_x64_start_reply.guest_rip;
    uint64_t ept_pml4 = m.hv_x64_start_reply.ept_pml4;

    struct guest_regs initial_regs;
    bzero(&initial_regs, sizeof(initial_regs));
    initial_regs.rbx = m.hv_x64_start_reply.initial_rbx;

    CURRENT_VMX.vmcs = &vmcs_areas[CURRENT->tid];
    CURRENT_VMX.saved_msrs = &saved_msrs[CURRENT->tid];
    CURRENT_VMX.saved_msrs->num_entries = 0;
    CURRENT_VMX.long_mode = false;
    CURRENT_VMX.pci_addr = 0;
    CURRENT_VMX.pending_irq_bitmap = 0;
    CURRENT_VMX.pics.master.init_phase = 0;
    CURRENT_VMX.pics.master.vector_base = 0;
    CURRENT_VMX.pics.master.irq_mask = 0;
    CURRENT_VMX.pics.slave.init_phase = 0;
    CURRENT_VMX.pics.slave.vector_base = 0;
    CURRENT_VMX.pics.slave.irq_mask = 0;
    CURRENT_VMX.launched = true;
    init_vmcs(CURRENT_VMX.vmcs, CURRENT_VMX.saved_msrs, guest_rip, ept_pml4);
    init_msrs();
    vmwrite_cpu_locals();

    unlock();
    ASSERT_VM_INST(asm_vmlaunch(&initial_regs));
    UNREACHABLE();
}

void x64_hv_init(void) {
    // TODO: Check if the CPU support VMX
    // TODO: Check the size of vmcs/vmx areas

    asm_write_cr4(asm_read_cr4() | CR4_VMXE);

    // Enable VMX.
    bzero(vmx_area, sizeof(vmx_area));
    *((uint32_t *) vmx_area) = asm_rdmsr(MSR_IA32_VMX_BASIC);
    ASSERT_VM_INST(asm_vmxon(into_paddr(vmx_area)));
}
