#include <ipc.h>
#include <printk.h>
#include <process.h>
#include <resea_idl.h>
#include <server.h>
#include <thread.h>
#include <types.h>

struct channel *kernel_server_ch = NULL;

/// The user pager. When a page fault occurred in vm areas that are registered
/// with this function, the kernel invokes this function to fill the page.
static paddr_t user_pager(struct vmarea *vma, vaddr_t vaddr) {
    struct message *ipc_buffer = CURRENT->kernel_ipc_buffer;
    struct channel *pager = vma->arg;
    // TRACE("user pager=%d, addr=%p", pager->cid, vaddr);

    // Construct a full_page_request message.
    struct fill_page_request_msg *m =
        (struct fill_page_request_msg *) ipc_buffer;
    m->header = FILL_PAGE_REQUEST_HEADER;
    m->pid = CURRENT->process->pid;
    m->addr = vaddr;

    // Invoke the user pager. This would blocks the current thread.
    sys_ipc(pager->cid, IPC_SEND | IPC_RECV | IPC_FROM_KERNEL);

    // The user pager replied the message.
    if (MSG_TYPE(ipc_buffer->header) < 0) {
        WARN("user pager returned an error");
        return 0;
    }

    struct fill_page_request_reply_msg *r =
        (struct fill_page_request_reply_msg *) ipc_buffer;
    paddr_t paddr = PAGE_PAYLOAD_ADDR(r->page);
    // TRACE("received a page from the pager: addr=%p", paddr);
    return paddr;
}

static error_t handle_printchar_msg(uint8_t ch) {
    arch_putchar(ch);
    return OK;
}

static NORETURN void handle_exit_current_msg(UNUSED int code) {
    // TODO: Kill the sender process.
    UNIMPLEMENTED();
}

static error_t handle_create_process_msg(struct process *sender,
                                         const char *name,
                                         struct create_process_reply_msg *r) {
    TRACE("kernel: create_process()");
    struct process *proc = process_create(name);
    if (!proc) {
        return ERR_OUT_OF_RESOURCE;
    }

    struct channel *user_ch = channel_create(proc);
    if (!user_ch) {
        process_destroy(proc);
        return ERR_OUT_OF_RESOURCE;
    }

    struct channel *pager_ch = channel_create(sender);
    if (!pager_ch) {
        channel_decref(pager_ch);
        process_destroy(proc);
        return ERR_OUT_OF_RESOURCE;
    }

    channel_link(user_ch, pager_ch);

    r->header = CREATE_PROCESS_REPLY_HEADER;
    r->pid = proc->pid;
    r->pager_ch = pager_ch->cid;
    TRACE("kernel: create_process_response(pid=%d, pager_ch=%d)", r->pid,
        r->pager_ch);
    return OK;
}

static error_t handle_spawn_thread_msg(pid_t pid, vaddr_t start, vaddr_t stack,
    vaddr_t buffer, vaddr_t arg, struct spawn_thread_reply_msg *r) {
    TRACE("kernel: spawn_thread(pid=%d, start=%p)", pid, start);

    struct process *proc = table_get(&all_processes, pid);
    if (!proc) {
        return ERR_INVALID_MESSAGE;
    }

    struct thread *thread = thread_create(proc, start, stack, buffer,
                                          (void *) arg);
    if (!thread) {
        return ERR_OUT_OF_RESOURCE;
    }

    thread_resume(thread);

    TRACE("kernel: spawn_thread_response(tid=%d)", thread->tid);
    r->header = SPAWN_THREAD_REPLY_HEADER;
    r->tid = thread->tid;
    return OK;
}

static error_t handle_add_pager_msg(pid_t pid, cid_t pager, vaddr_t start,
    vaddr_t size, uint8_t flags, struct add_pager_reply_msg *r) {
    TRACE("kernel: add_pager(pid=%d, pager=%d, range=%p-%p)", pid, pager, start,
        start + size);

    struct process *proc = table_get(&all_processes, pid);
    if (!proc) {
        WARN("invalid proc");
        return ERR_INVALID_MESSAGE;
    }

    struct channel *pager_ch = table_get(&proc->channels, pager);
    if (!pager_ch) {
        WARN("invalid pger_ch %d", pager);
        process_destroy(proc);
        return ERR_INVALID_MESSAGE;
    }

    error_t err = vmarea_add(proc, start, start + size, user_pager, pager_ch,
                             flags | PAGE_USER);
    if (err != OK) {
        WARN("failed to add a vm area: %d", err);
        process_destroy(proc);
        return err;
    }
    channel_incref(pager_ch);

    TRACE("kernel: add_pager_response()");
    r->header = ADD_PAGER_REPLY_HEADER;
    return OK;
}

static error_t handle_add_kernel_channel_msg(pid_t pid,
    struct add_kernel_channel_reply_msg *r) {
    TRACE("kernel: add_kernel_channel(pid=%d)", pid);

    struct process *proc = table_get(&all_processes, pid);
    if (!proc) {
        WARN("invalid pid");
        return ERR_INVALID_MESSAGE;
    }


    struct channel *kernel_ch = channel_create(kernel_process);
    if (!kernel_ch) {
        PANIC("failed to create a channel");
    }

    struct channel *user_ch = channel_create(proc);
    if (!user_ch) {
        PANIC("failed to create a channel");
    }

    channel_transfer(kernel_ch, kernel_server_ch);
    channel_link(kernel_ch, user_ch);

    r->header = ADD_KERNEL_CHANNEL_HEADER;
    r->kernel_ch = user_ch->cid;
    return OK;
}

struct irq_listener {
    uint8_t irq;
    struct channel *ch;
};

#define IRQ_LISTENERS_MAX 32
static struct irq_listener irq_listeners[IRQ_LISTENERS_MAX];

void deliver_interrupt(uint8_t irq) {
    for (int i = 0; i < IRQ_LISTENERS_MAX; i++) {
        if (irq_listeners[i].irq == irq && irq_listeners[i].ch) {
            channel_notify(irq_listeners[i].ch, NOTIFY_OP_ADD, 1);
        }
    }
}

static error_t handle_listen_irq_msg(cid_t cid, uint8_t irq,
                                     struct listen_irq_reply_msg *r) {
    struct channel *ch = table_get(&CURRENT->process->channels, cid);
    ASSERT(ch);

    INFO("kernel: listen_irq(ch=%pC, irq=%d)", ch, irq);
    r->header = LISTEN_IRQ_REPLY_HEADER;

    for (int i = 0; i < IRQ_LISTENERS_MAX; i++) {
        if (irq_listeners[i].ch == NULL) {
            irq_listeners[i].irq = irq;
            irq_listeners[i].ch = ch;
            enable_irq(irq);
            return OK;
        }
    }

    return ERR_NO_MEMORY;
}

error_t handle_allow_io_msg(struct process *sender,
                            struct allow_io_reply_msg *r) {
    INFO("kernel: allow_io(proc=%s)", sender->name);
    LIST_FOR_EACH(node, &sender->threads) {
        struct thread *thread = LIST_CONTAINER(thread, next, node);
        thread_allow_io(thread);
    }

    r->header = ALLOW_IO_REPLY_HEADER;
    return OK;
}

error_t handle_send_channel_to_process_msg(pid_t pid, cid_t cid,
        struct send_channel_to_process_reply_msg *r) {
    TRACE("kernel: send_channel_to_pid(pid=%d)", pid);

    struct process *proc = table_get(&all_processes, pid);
    if (!proc) {
        WARN("invalid proc");
        return ERR_INVALID_MESSAGE;
    }

    struct channel *ch = table_get(&CURRENT->process->channels, cid);
    struct channel *dst_ch = channel_create(proc);
    // FIXME: Clean up and return an error instead.
    ASSERT(ch);
    ASSERT(dst_ch);

    channel_link(ch->linked_to, dst_ch);
    channel_decref(ch);

    TRACE("kernel: send_channel_to_pid: created %pC", dst_ch);
    r->header = SEND_CHANNEL_TO_PROCESS_REPLY_HEADER;
    return OK;
}

NORETURN static void handle_exit_kernel_test_msg(void) {
    INFO("Power off");
    arch_poweroff();
}

/// The kernel server mainloop.
NORETURN static void mainloop(cid_t server_ch) {
    struct message *ipc_buffer = CURRENT->kernel_ipc_buffer;
    sys_ipc(server_ch, IPC_RECV | IPC_FROM_KERNEL);

    while (1) {
        cid_t from = ipc_buffer->from;

        // Resolves the sender thread.
        struct channel *ch = table_get(&kernel_process->channels, from);
        ASSERT(ch != NULL);
        struct process *sender = ch->linked_to->process;

        error_t err;
        switch (MSG_TYPE(ipc_buffer->header)) {
        case PRINTCHAR_MSG: {
            struct printchar_msg *m = (struct printchar_msg *) ipc_buffer;
            err = handle_printchar_msg(m->ch);
            break;
        }
        case EXIT_CURRENT_MSG: {
            struct exit_current_msg *m = (struct exit_current_msg *) ipc_buffer;
            handle_exit_current_msg(m->code);
        }
        case CREATE_PROCESS_MSG: {
            struct create_process_msg *m =
                (struct create_process_msg *) ipc_buffer;
            err = handle_create_process_msg(sender, (const char *) &m->path,
                (struct create_process_reply_msg *) ipc_buffer);
            break;
        }
        case ADD_KERNEL_CHANNEL_MSG: {
            struct add_kernel_channel_msg *m =
                (struct add_kernel_channel_msg *) ipc_buffer;
            err = handle_add_kernel_channel_msg(m->pid,
                (struct add_kernel_channel_reply_msg *) ipc_buffer);
            break;
        }
        case SPAWN_THREAD_MSG: {
            struct spawn_thread_msg *m = (struct spawn_thread_msg *) ipc_buffer;
            err = handle_spawn_thread_msg(m->pid, m->start, m->stack, m->buffer,
                m->arg, (struct spawn_thread_reply_msg *) ipc_buffer);
            break;
        }
        case ADD_PAGER_MSG: {
            struct add_pager_msg *m = (struct add_pager_msg *) ipc_buffer;
            err = handle_add_pager_msg(m->pid, m->pager, m->start, m->size,
                m->flags, (struct add_pager_reply_msg *) ipc_buffer);
            break;
        }
        case LISTEN_IRQ_MSG: {
            struct listen_irq_msg *m = (struct listen_irq_msg *) ipc_buffer;
            err = handle_listen_irq_msg(m->ch, m->irq,
                 (struct listen_irq_reply_msg *) ipc_buffer);
            break;
        }
        case ALLOW_IO_MSG:
            err = handle_allow_io_msg(sender,
                                      (struct allow_io_reply_msg *) ipc_buffer);
            break;
        case SEND_CHANNEL_TO_PROCESS_MSG: {
            struct send_channel_to_process_msg *m = (struct send_channel_to_process_msg *) ipc_buffer;
            err = handle_send_channel_to_process_msg(m->pid, m->ch,
                 (struct send_channel_to_process_reply_msg *) ipc_buffer);
            break;
        }
        case EXIT_KERNEL_TEST_MSG:
            handle_exit_kernel_test_msg();
            break;
        default:
            WARN("invalid message type %x", MSG_TYPE(ipc_buffer->header));
            err = ERR_INVALID_MESSAGE;
            ipc_buffer->header = ERROR_TO_HEADER(err);
        }

        if (err == ERR_DONT_REPLY) {
            sys_ipc(server_ch, IPC_RECV | IPC_FROM_KERNEL);
        } else {
            sys_ipc(from, IPC_SEND | IPC_RECV | IPC_FROM_KERNEL);
        }
    }
}

/// The kernel server thread entrypoint.
static void kernel_server_main(void) {
    ASSERT(CURRENT->process == kernel_process);

    for (int i = 0; i < IRQ_LISTENERS_MAX; i++) {
        irq_listeners[i].ch = NULL;
    }

    mainloop(kernel_server_ch->cid);
}

/// Initializes the kernel server.
void kernel_server_init(void) {
    kernel_server_ch = channel_create(kernel_process);
    struct thread *thread = thread_create(kernel_process,
                                          (vaddr_t) kernel_server_main,
                                          0 /* stack */,
                                          0 /* buffer */,
                                          0 /* arg */);
    thread_resume(thread);
}
