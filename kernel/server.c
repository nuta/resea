#include <channel.h>
#include <printk.h>
#include <process.h>
#include <idl_messages.h>
#include <server.h>
#include <thread.h>
#include <timer.h>
#include <table.h>
#include <types.h>
#include <syscall.h>

struct channel *kernel_server_ch = NULL;
static struct table user_timers;
static struct table irq_listeners;
static struct list_head active_irq_listeners;

static struct process *get_sender_process(cid_t from) {
    // Resolves the sender thread.
    struct channel *ch = table_get(&kernel_process->channels, from);
    ASSERT(ch != NULL);
    return ch->linked_with->process;
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

static error_t handle_runtime_exit_current(struct message *m) {
    process_destroy(get_sender_process(m->from));
    return ERR_DONT_REPLY;
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
        channel_destroy(pager_ch);
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

    struct process *proc = table_get(&process_table, pid);
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

    struct process *proc = table_get(&process_table, pid);
    if (!proc) {
        WARN("Invalid pid #%d.", pid);
        return ERR_INVALID_MESSAGE;
    }

    struct channel *pager_ch = table_get(&proc->channels, pager);
    if (!pager_ch) {
        WARN("Invalid pager_ch @%d.", pager);
        process_destroy(proc);
        return ERR_INVALID_MESSAGE;
    }

    error_t err = vmarea_add(proc, start, start + size, user_pager, pager_ch,
                             flags | PAGE_USER);
    if (err != OK) {
        WARN("Failed to add a vm area: %vE.", err);
        process_destroy(proc);
        return err;
    }
    channel_incref(pager_ch);

    TRACE("kernel: add_pager_response()");
    m->header = PROCESS_ADD_PAGER_REPLY_HEADER;
    return OK;
}

static error_t handle_server_connect(struct message *m) {
    struct channel *ch = channel_create(kernel_process);
    if (!ch) {
        return ERR_NO_MEMORY;
    }

    channel_transfer(ch, kernel_server_ch);
    m->header = SERVER_CONNECT_REPLY_HEADER;
    m->payloads.server.connect_reply.ch = ch->cid;
    return OK;
}

void deliver_interrupt(uint8_t irq) {
    LIST_FOR_EACH(listener, &active_irq_listeners, struct irq_listener, next) {
        if (listener->irq == irq) {
            channel_notify(listener->ch, NOTIFY_INTERRUPT);
        }
    }
}

static error_t handle_io_listen_irq(struct message *m) {
    cid_t cid   = m->payloads.io.listen_irq.ch;
    uint8_t irq = m->payloads.io.listen_irq.irq;

    struct channel *ch = table_get(&CURRENT->process->channels, cid);
    ASSERT(ch);

    TRACE("kernel: listen_irq(ch=%pC, irq=%d)", ch, irq);

    int id = table_alloc(&irq_listeners);
    if (!id) {
        return ERR_NO_MEMORY;
    }

    struct irq_listener *listener = KMALLOC(&small_arena,
                                            sizeof(struct irq_listener));
    if (!listener) {
        table_free(&irq_listeners, id);
        return ERR_NO_MEMORY;
    }

    listener->irq = irq;
    listener->ch  = ch;
    table_set(&irq_listeners, id, listener);
    list_push_back(&active_irq_listeners, &listener->next);
    enable_irq(irq);

    m->header = IO_LISTEN_IRQ_REPLY_HEADER;
    return OK;
}

static error_t handle_io_allow_iomapped_io(struct message *m) {
    struct process *sender = get_sender_process(m->from);
    TRACE("kernel: allow_io(proc=%s)", sender->name);

    LIST_FOR_EACH(thread, &sender->threads, struct thread, next) {
        thread_allow_io(thread);
    }

    m->header = IO_ALLOW_IOMAPPED_IO_REPLY_HEADER;
    return OK;
}

static error_t handle_process_send_channel(struct message *m) {
    pid_t pid = m->payloads.process.send_channel.pid;
    cid_t cid = m->payloads.process.send_channel.ch;
    TRACE("kernel: send_channel_to_pid(pid=%d)", pid);

    struct process *proc = table_get(&process_table, pid);
    if (!proc) {
        WARN("Invalid pid #%d.", pid);
        return ERR_INVALID_MESSAGE;
    }

    struct channel *ch = table_get(&CURRENT->process->channels, cid);
    ASSERT(ch);

    struct channel *dst_ch = channel_create(proc);
    if (!dst_ch) {
        channel_destroy(ch);
        return ERR_NO_MEMORY;
    }

    channel_link(ch->linked_with, dst_ch);
    channel_destroy(ch);

    TRACE("kernel: send_channel_to_pid: created %pC", dst_ch);
    m->header = PROCESS_SEND_CHANNEL_REPLY_HEADER;
    return OK;
}

static void user_timer_handler(struct timer *timer) {
    struct channel *ch = timer->arg;
    channel_notify(ch, NOTIFY_TIMER);
    if (!timer->interval) {
        channel_destroy(timer->arg);
        table_free(&user_timers, timer->id);
    }
}

static error_t handle_timer_set(struct message *m) {
    cid_t cid = m->payloads.timer.set.ch;
    int32_t initial = m->payloads.timer.set.initial;
    int32_t interval = m->payloads.timer.set.interval;

    struct channel *ch = table_get(&CURRENT->process->channels, cid);
    ASSERT(ch);

    int timer_id = table_alloc(&user_timers);
    if (!timer_id) {
        return ERR_NO_MEMORY;
    }

    struct timer *timer = timer_create(initial, interval, user_timer_handler,
                                       ch);
    if (!timer) {
        table_free(&user_timers, timer_id);
        return ERR_NO_MEMORY;
    }

    table_set(&user_timers, timer_id, timer);
    channel_incref(ch);

    m->header = TIMER_SET_REPLY_HEADER;
    m->payloads.timer.set_reply.timer = timer_id;
    return OK;
}

static error_t handle_timer_clear(UNUSED struct message *m) {
    int timer_id = m->payloads.timer.clear.timer;

    struct timer *timer = table_get(&user_timers, timer_id);
    if (!timer) {
        return ERR_INVALID_PAYLOAD;
    }

    channel_destroy(timer->arg);
    table_free(&user_timers, timer->id);
    timer_destroy(timer);

    m->header = TIMER_CLEAR_REPLY_HEADER;
    return OK;
}

static error_t process_message(struct message *m) {
    switch (MSG_TYPE(m->header)) {
    case RUNTIME_EXIT_CURRENT_MSG: return handle_runtime_exit_current(m);
    case RUNTIME_PRINTCHAR_MSG:    return handle_runtime_printchar(m);
    case PROCESS_CREATE_MSG:       return handle_process_create(m);
    case PROCESS_DESTROY_MSG:      return handle_process_destroy(m);
    case PROCESS_ADD_PAGER_MSG:    return handle_process_add_pager(m);
    case PROCESS_SEND_CHANNEL_MSG: return handle_process_send_channel(m);
    case THREAD_SPAWN_MSG:         return handle_thread_spawn(m);
    case THREAD_DESTROY_MSG:       return handle_thread_destroy(m);
    case IO_ALLOW_IOMAPPED_IO_MSG: return handle_io_allow_iomapped_io(m);
    case IO_LISTEN_IRQ_MSG:        return handle_io_listen_irq(m);
    case TIMER_SET_MSG:            return handle_timer_set(m);
    case TIMER_CLEAR_MSG:          return handle_timer_clear(m);
    case SERVER_CONNECT_MSG:       return handle_server_connect(m);
    }

    return ERR_UNEXPECTED_MESSAGE;
}

/// The kernel server mainloop.
NORETURN static void mainloop(cid_t server_ch) {
    struct message *m = CURRENT->kernel_ipc_buffer;
    while (1) {
        sys_ipc(server_ch, IPC_RECV | IPC_FROM_KERNEL);

        error_t err = process_message(m);
        if (err == ERR_DONT_REPLY) {
            continue;
        }

        if (err != OK) {
            m->header = ERROR_TO_HEADER(err);
        }
        sys_ipc(m->from, IPC_SEND | IPC_FROM_KERNEL);
    }
}

/// The kernel server thread entrypoint.
static void kernel_server_main(void) {
    ASSERT(CURRENT->process == kernel_process);
    table_init(&user_timers);
    table_init(&irq_listeners);
    list_init(&active_irq_listeners);
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
