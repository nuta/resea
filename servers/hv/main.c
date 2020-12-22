#include <resea/ipc.h>
#include <resea/async.h>
#include <resea/timer.h>
#include <string.h>
#include "guest.h"
#include "ioport.h"
#include "mm.h"

extern char __kernel_elf[];
extern char __kernel_elf_end[];

#define SENDER_CHECK()                                                         \
    do {                                                                       \
        if (m.src != KERNEL_TASK) {                                            \
            WARN_DBG("%s: not from the kernel: task=%d",                       \
                     msgtype2str(m.type), m.src);                              \
            ipc_reply_err(m.src, ERR_NOT_PERMITTED);                           \
            break;                                                             \
        }                                                                      \
    } while (0)

void main(void) {
    TRACE("starting...");

    guest_init();

    size_t __kernel_elf_len = (size_t) __kernel_elf_end - (size_t) __kernel_elf;
    ASSERT_OK(guest_create((uint8_t *) __kernel_elf, __kernel_elf_len));

    INFO("ready");
    while (true) {
        guest_send_pending_events();

        struct message m;
        bzero(&m, sizeof(m));
        ASSERT_OK(ipc_recv(IPC_ANY, &m));

        timer_set(1);
        switch (m.type) {
            case NOTIFICATIONS_MSG:
                SENDER_CHECK();
                if (m.notifications.data & NOTIFY_TIMER) {
                    timer_set(1);
                    guest_tick();
                }
                break;
            case HV_AWAIT_MSG:
                SENDER_CHECK();
                struct guest *guest = guest_lookup(m.hv_await.task);
                async_reply(guest->task);
                m.type = HV_INJECT_IRQ_MSG;
                m.hv_inject_irq.irq_bitmap = guest->pending_irq_bitmap;
                ipc_reply(guest->task, &m);

                guest->pending_irq_bitmap = 0;
                break;
            case HV_X64_START_MSG: {
                SENDER_CHECK();
                if (m.src != KERNEL_TASK) {
                    WARN_DBG("hv.x64_start: not from the kernel: task=%d", m.src);
                    ipc_reply_err(m.src, ERR_NOT_PERMITTED);
                    break;
                }

                struct guest *guest = guest_lookup(m.hv_x64_start.task);

                m.type = HV_X64_START_REPLY_MSG;
                m.hv_x64_start_reply.guest_rip = guest->entry;
                m.hv_x64_start_reply.ept_pml4 = guest->ept_pml4_paddr;
                m.hv_x64_start_reply.initial_rbx = guest->initial_regs.rbx;
                ipc_reply(guest->task, &m);
                break;
            }
            case HV_IOPORT_READ_MSG: {
                SENDER_CHECK();
                struct guest *guest = guest_lookup(m.hv_ioport_read.task);
                uint32_t value = handle_io_read(
                    guest,
                    m.hv_ioport_read.port,
                    m.hv_ioport_read.size
                );

                m.type = HV_IOPORT_READ_REPLY_MSG;
                m.hv_ioport_read_reply.value = value;
                ipc_reply(guest->task, &m);
                break;
            }
            case HV_IOPORT_WRITE_MSG: {
                SENDER_CHECK();
                struct guest *guest = guest_lookup(m.hv_ioport_write.task);
                handle_io_write(
                    guest,
                    m.hv_ioport_write.port,
                    m.hv_ioport_write.size,
                    m.hv_ioport_write.value
                );

                m.type = HV_IOPORT_WRITE_REPLY_MSG;
                ipc_reply(guest->task, &m);
                break;
            }
            case HV_GUEST_PAGE_FAULT_MSG: {
                SENDER_CHECK();
                hv_frame_t frame;
                memcpy(&frame, &m.hv_guest_page_fault.frame, sizeof(frame));

                struct guest *guest = guest_lookup(m.hv_guest_page_fault.task);
                handle_ept_fault(guest, m.hv_guest_page_fault.gpaddr,
                                 &frame);

                m.type = HV_GUEST_PAGE_FAULT_REPLY_MSG;
                memcpy(&m.hv_guest_page_fault_reply.frame, &frame, sizeof(frame));
                ipc_reply(guest->task, &m);
                break;
            }
            case HV_HALT_MSG: {
                SENDER_CHECK();
                struct guest *guest = guest_lookup(m.hv_ioport_write.task);
                guest->halted = async_is_empty(guest->task);
                if (!guest->halted) {
                    guest->halted = false;
                    m.type = HV_HALT_REPLY_MSG;
                    ipc_reply(guest->task, &m);
                }
                break;
            }
            default:
                discard_unknown_message(&m);
        }
    }
}
