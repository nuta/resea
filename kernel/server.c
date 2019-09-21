#include <ipc.h>
#include <printk.h>
#include <process.h>
#include <resea_idl.h>
#include <server.h>
#include <thread.h>
#include <types.h>

struct channel *kernel_server_ch = NULL;

static struct process *get_sender_process(cid_t from) {
    // Resolves the sender thread.
    struct channel *ch = table_get(&kernel_process->channels, from);
    ASSERT(ch != NULL);
    return ch->linked_to->process;
}

/// The user pager. When a page fault occurred in vm areas that are registered
/// with this function, the kernel invokes this function to fill the page.
static paddr_t user_pager(struct vmarea *vma, vaddr_t vaddr) {
    struct message *m = CURRENT->kernel_ipc_buffer;
    struct channel *pager = vma->arg;
    // TRACE("user pager=%d, addr=%p", pager->cid, vaddr);

    // Construct a pager.fill message.
    m->header = PAGER_FILL_HEADER;
    m->payloads.pager.fill.pid = CURRENT->process->pid;
    m->payloads.pager.fill.addr = vaddr;

    // Invoke the user pager. This would blocks the current thread.
    sys_ipc(pager->cid, IPC_SEND | IPC_RECV | IPC_FROM_KERNEL);

    // The user pager replied the message.
    if (MSG_TYPE(m->header) < 0) {
        WARN("user pager returned an error");
        return 0;
    }

    paddr_t paddr = PAGE_PAYLOAD_ADDR(m->payloads.pager.fill_reply.page);
    // TRACE("received a page from the pager: addr=%p", paddr);
    return paddr;
}

static error_t handle_runtime_printchar(struct message *m) {
    arch_putchar(m->payloads.runtime.printchar.ch);
    return OK;
}

static error_t handle_runtime_exit_current(UNUSED struct message *m) {
    // TODO: Kill the sender process.
    UNIMPLEMENTED();
}

static error_t handle_process_create(struct message *m) {
    const char *name = m->payloads.process.create.name;
    TRACE("kernel: create_process(name=%s)", name);

    struct process *proc = process_create(name);
    if (!proc) {
        return ERR_OUT_OF_RESOURCE;
    }

    struct channel *user_ch = channel_create(proc);
    if (!user_ch) {
        process_destroy(proc);
        return ERR_OUT_OF_RESOURCE;
    }

    struct channel *pager_ch = channel_create(get_sender_process(m->from));
    if (!pager_ch) {
        channel_decref(pager_ch);
        process_destroy(proc);
        return ERR_OUT_OF_RESOURCE;
    }

    channel_link(user_ch, pager_ch);

    m->header = PROCESS_CREATE_REPLY_HEADER;
    m->payloads.process.create_reply.pid = proc->pid;
    m->payloads.process.create_reply.pager_ch = pager_ch->cid;
    return OK;
}

static error_t handle_process_destroy(UNUSED struct message *m) {
    UNIMPLEMENTED();
}

static error_t handle_thread_spawn(struct message *m) {
    pid_t pid        = m->payloads.thread.spawn.pid;
    uintptr_t start  = m->payloads.thread.spawn.start;
    uintptr_t stack  = m->payloads.thread.spawn.stack;
    uintptr_t buffer = m->payloads.thread.spawn.buffer;
    uintptr_t arg    = m->payloads.thread.spawn.arg;

    struct process *proc = table_get(&all_processes, pid);
    if (!proc) {
        return ERR_INVALID_MESSAGE;
    }

    struct thread *thread;
    thread = thread_create(proc, start, stack, buffer, (void *) arg);
    if (!thread) {
        return ERR_OUT_OF_RESOURCE;
    }

    thread_resume(thread);

    TRACE("kernel: spawn_thread_response(tid=%d)", thread->tid);
    m->header = THREAD_SPAWN_REPLY_HEADER;
    m->payloads.thread.spawn_reply.tid = thread->tid;
    return OK;
}

static error_t handle_thread_destroy(UNUSED struct message *m) {
    UNIMPLEMENTED();
}

static error_t handle_process_add_pager(struct message *m) {
    pid_t pid       = m->payloads.process.add_pager.pid;
    cid_t pager     = m->payloads.process.add_pager.pager;
    uintptr_t start = m->payloads.process.add_pager.start;
    size_t size     = m->payloads.process.add_pager.size;
    uint8_t flags   = m->payloads.process.add_pager.flags;

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
    m->header = PROCESS_ADD_PAGER_REPLY_HEADER;
    return OK;
}

static error_t handle_process_add_kernel_channel(struct message *m) {
    pid_t pid = m->payloads.process.add_kernel_channel.pid;
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

    m->header = PROCESS_ADD_KERNEL_CHANNEL_HEADER;
    m->payloads.process.add_kernel_channel_reply.kernel_ch = user_ch->cid;
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
            channel_notify(irq_listeners[i].ch, NOTIFY_INTERRUPT);
        }
    }
}

static error_t handle_io_listen_irq(struct message *m) {
    cid_t cid   = m->payloads.io.listen_irq.ch;
    uint8_t irq = m->payloads.io.listen_irq.irq;

    struct channel *ch = table_get(&CURRENT->process->channels, cid);
    ASSERT(ch);

    TRACE("kernel: listen_irq(ch=%pC, irq=%d)", ch, irq);

    for (int i = 0; i < IRQ_LISTENERS_MAX; i++) {
        if (irq_listeners[i].ch == NULL) {
            irq_listeners[i].irq = irq;
            irq_listeners[i].ch = ch;
            enable_irq(irq);
            return OK;
        }
    }

    m->header = IO_LISTEN_IRQ_REPLY_HEADER;
    return ERR_NO_MEMORY;
}

static error_t handle_io_allow_iomapped_io(struct message *m) {
    struct process *sender = get_sender_process(m->from);
    TRACE("kernel: allow_io(proc=%s)", sender->name);
    
    LIST_FOR_EACH(node, &sender->threads) {
        struct thread *thread = LIST_CONTAINER(thread, next, node);
        thread_allow_io(thread);
    }

    m->header = IO_ALLOW_IOMAPPED_IO_REPLY_HEADER;
    return OK;
}

static error_t handle_process_send_channel_to_process(struct message *m) {
    pid_t pid = m->payloads.process.send_channel_to_process.pid;
    cid_t cid = m->payloads.process.send_channel_to_process.ch;
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
    m->header = PROCESS_SEND_CHANNEL_TO_PROCESS_REPLY_HEADER;
    return OK;
}

NORETURN static void handle_kernel_exit_kernel_test(UNUSED struct message *m) {
    INFO("Power off");
    arch_poweroff();
}

static error_t process_message(struct message *m) {
    switch (MSG_TYPE(m->header)) {
    case RUNTIME_EXIT_CURRENT_MSG:       return handle_runtime_exit_current(m);
    case RUNTIME_PRINTCHAR_MSG:          return handle_runtime_printchar(m);
    case PROCESS_CREATE_MSG:             return handle_process_create(m);
    case PROCESS_DESTROY_MSG:            return handle_process_destroy(m);
    case PROCESS_ADD_PAGER_MSG:          return handle_process_add_pager(m);
    case PROCESS_SEND_CHANNEL_TO_PROCESS_MSG: return handle_process_send_channel_to_process(m);
    case PROCESS_ADD_KERNEL_CHANNEL_MSG: return handle_process_add_kernel_channel(m);
    case THREAD_SPAWN_MSG:               return handle_thread_spawn(m);
    case THREAD_DESTROY_MSG:             return handle_thread_destroy(m);
    case IO_ALLOW_IOMAPPED_IO_MSG:       return handle_io_allow_iomapped_io(m);
    case IO_LISTEN_IRQ_MSG:              return handle_io_listen_irq(m);
    case KERNEL_EXIT_KERNEL_TEST_MSG:
        handle_kernel_exit_kernel_test(m);
    }
    return ERR_UNEXPECTED_MESSAGE;
}

/// The kernel server mainloop.
NORETURN static void mainloop(cid_t server_ch) {
    struct message *m = CURRENT->kernel_ipc_buffer;
    sys_ipc(server_ch, IPC_RECV | IPC_FROM_KERNEL);

    while (1) {
        error_t err = process_message(m);
        if (err != OK) {
            m->header = ERROR_TO_HEADER(err);
        }

        if (err == ERR_DONT_REPLY) {
            sys_ipc(server_ch, IPC_RECV | IPC_FROM_KERNEL);
        } else {
            sys_ipc(m->from, IPC_SEND | IPC_RECV | IPC_FROM_KERNEL);
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
