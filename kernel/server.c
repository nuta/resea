#include <channel.h>
#include <support/printk.h>
#include <support/stats.h>
#include <process.h>
#include <server.h>
#include <thread.h>
#include <timer.h>
#include <table.h>
#include <types.h>
#include <ipc.h>
#include <arch.h>

#define PAGER_CID ((cid_t) 1)

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
    m->header = PAGER_FILL_MSG;
    m->payloads.pager.fill.proc = CURRENT->process->pid;
    m->payloads.pager.fill.addr = vaddr;
    m->payloads.pager.fill.num_pages = 1;

    // Invoke the user pager. This would blocks the current thread.
    error_t err = kernel_ipc(pager->cid, IPC_SEND | IPC_RECV);
    if (err != OK) {
        WARN("failed to call the user pager (error=%d)", err);
        return 0;
    }

    // I suppose we never receive notifications on this channel.
    DEBUG_ASSERT(m->notification == 0);

    // The user pager replied the message.
    if (m->header != PAGER_FILL_REPLY_MSG) {
        int16_t type_or_error = m->header;
        if (type_or_error < 0) {
            WARN("user pager returned an error");
        } else {
            WARN("user pager replied an invalid header (header=%p, expected=%p)",
                 m->header, PAGER_FILL_REPLY_MSG);
        }
        return 0;
    }

    paddr_t paddr = m->payloads.pager.fill_reply.page;
    // TRACE("received a page from the pager: addr=%p", paddr);
    return paddr;
}

static error_t handle_runtime_printchar(struct message *m) {
    print_char(m->payloads.runtime.printchar.ch);
    return OK;
}

static error_t handle_runtime_print_str(struct message *m) {
    char *str = m->payloads.runtime.print_str.str;
    for (int i = 0; str[i] != '\0' && i < STRING_LEN_MAX; i++) {
        print_char(str[i]);
    }
    return OK;
}

static error_t handle_runtime_exit(struct message *m) {
    process_destroy(get_sender_process(m->from));
    return NO_REPLY;
}

static error_t handle_process_create(struct message *m) {
    const char *name = m->payloads.kernel.create_process.name;
    TRACE("kernel: create_process(name=%s)", name);

    struct process *proc = process_create(name);
    if (!proc) {
        return ERR_OUT_OF_RESOURCE;
    }

    struct channel *their_ch = channel_create(proc);
    if (!their_ch) {
        process_destroy(proc);
        return ERR_OUT_OF_RESOURCE;
    }

    DEBUG_ASSERT(their_ch->cid == PAGER_CID);

    struct channel *our_ch = channel_create(kernel_process);
    if (!our_ch) {
        channel_destroy(our_ch);
        process_destroy(proc);
        return ERR_OUT_OF_RESOURCE;
    }

    channel_link(our_ch, their_ch);

    m->header = KERNEL_CREATE_PROCESS_REPLY_MSG;
    m->payloads.kernel.create_process_reply.proc = proc->pid;
    m->payloads.kernel.create_process_reply.pager_ch = our_ch->cid;
    return OK;
}

static error_t handle_process_destroy(UNUSED struct message *m) {
    UNIMPLEMENTED();
}

static error_t handle_thread_spawn(struct message *m) {
    pid_t pid        = m->payloads.kernel.spawn_thread.proc;
    uintptr_t start  = m->payloads.kernel.spawn_thread.start;
    uintptr_t stack  = m->payloads.kernel.spawn_thread.stack;
    uintptr_t buffer = m->payloads.kernel.spawn_thread.buffer;
    uintptr_t arg    = m->payloads.kernel.spawn_thread.arg;

    struct process *proc = table_get(&process_table, pid);
    if (!proc) {
        return ERR_NOT_FOUND;
    }

    struct thread *thread;
    thread = thread_create(proc, start, stack, buffer, (void *) arg);
    if (!thread) {
        return ERR_OUT_OF_RESOURCE;
    }

    thread_resume(thread);

    TRACE("kernel: spawn_thread_response(tid=%d)", thread->tid);
    m->header = KERNEL_SPAWN_THREAD_REPLY_MSG;
    m->payloads.kernel.spawn_thread_reply.thread = thread->tid;
    return OK;
}

static error_t handle_thread_destroy(UNUSED struct message *m) {
    UNIMPLEMENTED();
}

static error_t handle_process_add_vm_area(struct message *m) {
    pid_t pid       = m->payloads.kernel.add_vm_area.proc;
    uintptr_t start = m->payloads.kernel.add_vm_area.start;
    size_t size     = m->payloads.kernel.add_vm_area.size;
    uint8_t flags   = m->payloads.kernel.add_vm_area.flags;

    struct process *proc = table_get(&process_table, pid);
    if (!proc) {
        WARN("Invalid pid #%d.", pid);
        return ERR_NOT_FOUND;
    }

    struct channel *pager_ch = table_get(&proc->channels, PAGER_CID);
    if (!pager_ch) {
        WARN("pager channel (@1) is closed.", PAGER_CID);
        process_destroy(proc);
        return ERR_NOT_FOUND;
    }

    error_t err = vmarea_create(proc, start, start + size, user_pager, pager_ch,
                                flags | PAGE_USER);
    if (err != OK) {
        WARN("Failed to add a vm area: %vE.", err);
        process_destroy(proc);
        return err;
    }
    channel_incref(pager_ch);

    TRACE("kernel: add_vm_area_response()");
    m->header = KERNEL_ADD_VM_AREA_REPLY_MSG;
    return OK;
}

static error_t handle_server_connect(struct message *m) {
    struct channel *ch = channel_create(kernel_process);
    if (!ch) {
        return ERR_OUT_OF_MEMORY;
    }

    channel_transfer(ch, kernel_server_ch);
    m->header = SERVER_CONNECT_REPLY_MSG;
    m->payloads.server.connect_reply.ch = ch->cid;
    return OK;
}

void deliver_interrupt(uint8_t irq) {
    LIST_FOR_EACH(listener, &active_irq_listeners, struct irq_listener, next) {
        if (listener->irq == irq) {
            OOPS_ON_ERROR(channel_notify(listener->ch, NOTIFY_INTERRUPT));
        }
    }
}

static error_t handle_listen_irq(struct message *m) {
    cid_t cid   = m->payloads.kernel.listen_irq.ch;
    uint8_t irq = m->payloads.kernel.listen_irq.irq;

    struct channel *ch = table_get(&CURRENT->process->channels, cid);
    ASSERT(ch);

    TRACE("kernel: listen_irq(ch=%pC, irq=%d)", ch, irq);

    int id = table_alloc(&irq_listeners);
    if (!id) {
        return ERR_OUT_OF_MEMORY;
    }

    struct irq_listener *listener = KMALLOC(&object_arena,
                                            sizeof(struct irq_listener));
    if (!listener) {
        table_free(&irq_listeners, id);
        return ERR_OUT_OF_MEMORY;
    }

    listener->irq = irq;
    listener->ch  = ch;
    table_set(&irq_listeners, id, listener);
    list_push_back(&active_irq_listeners, &listener->next);
    enable_irq(irq);

    m->header = KERNEL_LISTEN_IRQ_REPLY_MSG;
    return OK;
}

static error_t handle_process_inject_channel(struct message *m) {
    pid_t pid = m->payloads.kernel.inject_channel.proc;
    cid_t cid = m->payloads.kernel.inject_channel.ch;
    TRACE("kernel: inject_channel_to_pid(pid=%d)", pid);

    struct process *proc = table_get(&process_table, pid);
    if (!proc) {
        WARN("Invalid pid #%d.", pid);
        return ERR_NOT_FOUND;
    }

    struct channel *ch = table_get(&CURRENT->process->channels, cid);
    ASSERT(ch);

    struct channel *dst_ch = channel_create(proc);
    if (!dst_ch) {
        channel_destroy(ch);
        return ERR_OUT_OF_MEMORY;
    }

    channel_link(ch->linked_with, dst_ch);
    channel_destroy(ch);

    TRACE("kernel: inject_channel_to_pid: created %pC", dst_ch);
    m->header = KERNEL_INJECT_CHANNEL_REPLY_MSG;
    return OK;
}

static void user_timer_handler(struct timer *timer) {
    struct channel *ch = timer->arg;
    OOPS_ON_ERROR(channel_notify(ch, NOTIFY_TIMER));
}

static error_t handle_read_io_port(struct message *m) {
    vaddr_t addr = m->payloads.kernel.read_ioport.addr;
    vaddr_t size = m->payloads.kernel.read_ioport.size;
    uint64_t data = arch_read_ioport(addr, size);
    m->header = KERNEL_READ_IOPORT_REPLY_MSG;
    m->payloads.kernel.read_ioport_reply.data = data;
    return OK;
}

static error_t handle_write_io_port(struct message *m) {
    vaddr_t addr = m->payloads.kernel.write_ioport.addr;
    vaddr_t size = m->payloads.kernel.write_ioport.size;
    uint64_t data = m->payloads.kernel.write_ioport.data;
    arch_write_ioport(addr, size, data);
    m->header = KERNEL_WRITE_IOPORT_REPLY_MSG;
    return OK;
}

static error_t handle_batch_write_io_port(struct message *m) {
#define HANDLE_NTH_WRITE(nth)                                                  \
    do {                                                                       \
        vaddr_t addr = m->payloads.kernel.batch_write_ioport.addr ## nth;      \
        vaddr_t size = m->payloads.kernel.batch_write_ioport.size ## nth;      \
        uint64_t data = m->payloads.kernel.batch_write_ioport.data ## nth;     \
        if (addr) {                                                            \
            arch_write_ioport(addr, size, data);                               \
        }                                                                      \
    } while (0)

    HANDLE_NTH_WRITE(1);
    HANDLE_NTH_WRITE(2);
    HANDLE_NTH_WRITE(3);
    HANDLE_NTH_WRITE(4);
    m->header = KERNEL_BATCH_WRITE_IOPORT_REPLY_MSG;
    return OK;
}

static error_t handle_get_screen_buffer(struct message *m) {
    paddr_t page;
    size_t num_pages;
    error_t err = arch_get_screen_buffer(&page, &num_pages);
    m->header = KERNEL_GET_SCREEN_BUFFER_REPLY_MSG;
    m->payloads.kernel.get_screen_buffer_reply.page = page;
    m->payloads.kernel.get_screen_buffer_reply.page_len = num_pages * PAGE_SIZE;
    return err;
}

static error_t handle_timer_create(struct message *m) {
    cid_t cid = m->payloads.timer.create.ch;
    int32_t initial = m->payloads.timer.create.initial;
    int32_t interval = m->payloads.timer.create.interval;

    struct channel *ch = table_get(&CURRENT->process->channels, cid);
    ASSERT(ch);

    int timer_id = table_alloc(&user_timers);
    if (!timer_id) {
        return ERR_OUT_OF_MEMORY;
    }

    channel_incref(ch);
    struct timer *timer = timer_create(initial, interval, user_timer_handler,
                                       ch);
    if (!timer) {
        table_free(&user_timers, timer_id);
        return ERR_OUT_OF_MEMORY;
    }

    table_set(&user_timers, timer_id, timer);

    m->header = TIMER_CREATE_REPLY_MSG;
    m->payloads.timer.create_reply.timer = timer_id;
    return OK;
}

static error_t handle_timer_reset(struct message *m) {
    handle_t handle = m->payloads.timer.reset.timer;
    int32_t initial = m->payloads.timer.reset.initial;
    int32_t interval = m->payloads.timer.reset.interval;

    struct timer *timer = table_get(&user_timers, handle);
    if (!timer) {
        return ERR_NOT_FOUND;
    }

    timer_reset(timer, initial, interval);
    m->header = TIMER_RESET_REPLY_MSG;
    return OK;
}

static error_t handle_timer_clear(UNUSED struct message *m) {
    int timer_id = m->payloads.timer.clear.timer;

    struct timer *timer = table_get(&user_timers, timer_id);
    if (!timer) {
        return ERR_INVALID_PAYLOAD;
    }

    channel_decref(timer->arg);
    table_free(&user_timers, timer->id);
    timer_destroy(timer);

    m->header = TIMER_CLEAR_REPLY_MSG;
    return OK;
}

static error_t handle_timer_uptime(UNUSED struct message *m) {
    m->header = TIMER_UPTIME_REPLY_MSG;
    m->payloads.timer.uptime_reply.uptime = timer_uptime();
    return OK;
}

static error_t handle_read_kernel_log(UNUSED struct message *m) {
    read_kernel_log(m->payloads.kernel.read_kernel_log_reply.str,
                    STRING_LEN_MAX);
    m->header = KERNEL_READ_KERNEL_LOG_REPLY_MSG;
    return OK;
}

static error_t handle_read_stats(UNUSED struct message *m) {
    m->header = KERNEL_READ_STATS_REPLY_MSG;
    m->payloads.kernel.read_stats_reply.uptime = timer_uptime();
    m->payloads.kernel.read_stats_reply.ipc_total = READ_STAT(ipc_total);
    m->payloads.kernel.read_stats_reply.page_fault_total =
        READ_STAT(page_fault_total);
    m->payloads.kernel.read_stats_reply.context_switch_total =
        READ_STAT(context_switch_total);
    m->payloads.kernel.read_stats_reply.kernel_call_total =
        READ_STAT(kernel_call_total);
    return OK;
}

static error_t process_message(struct message *m) {
    INC_STAT(kernel_call_total);
    switch (m->header) {
    case RUNTIME_EXIT_MSG:              return handle_runtime_exit(m);
    case RUNTIME_PRINTCHAR_MSG:         return handle_runtime_printchar(m);
    case RUNTIME_PRINT_STR_MSG:         return handle_runtime_print_str(m);
    case KERNEL_CREATE_PROCESS_MSG:     return handle_process_create(m);
    case KERNEL_DESTROY_PROCESS_MSG:    return handle_process_destroy(m);
    case KERNEL_ADD_VM_AREA_MSG:        return handle_process_add_vm_area(m);
    case KERNEL_INJECT_CHANNEL_MSG:     return handle_process_inject_channel(m);
    case KERNEL_SPAWN_THREAD_MSG:       return handle_thread_spawn(m);
    case KERNEL_DESTROY_THREAD_MSG:     return handle_thread_destroy(m);
    case KERNEL_LISTEN_IRQ_MSG:         return handle_listen_irq(m);
    case KERNEL_READ_IOPORT_MSG:        return handle_read_io_port(m);
    case KERNEL_WRITE_IOPORT_MSG:       return handle_write_io_port(m);
    case KERNEL_BATCH_WRITE_IOPORT_MSG: return handle_batch_write_io_port(m);
    case KERNEL_GET_SCREEN_BUFFER_MSG:  return handle_get_screen_buffer(m);
    case KERNEL_READ_KERNEL_LOG_MSG:    return handle_read_kernel_log(m);
    case KERNEL_READ_STATS_MSG:         return handle_read_stats(m);
    case TIMER_CREATE_MSG:              return handle_timer_create(m);
    case TIMER_RESET_MSG:               return handle_timer_reset(m);
    case TIMER_CLEAR_MSG:               return handle_timer_clear(m);
    case TIMER_UPTIME_MSG:              return handle_timer_uptime(m);
    case SERVER_CONNECT_MSG:            return handle_server_connect(m);
    }

    WARN("unknown message: header=%p", m->header);
    return ERR_UNKNOWN_MESSAGE;
}

/// The kernel server mainloop.
NORETURN static void mainloop(cid_t server_ch) {
    struct message *m = CURRENT->kernel_ipc_buffer;
    while (1) {
        error_t err = kernel_ipc(server_ch, IPC_RECV);
        if (err == ERR_NEEDS_RETRY) {
            continue;
        }
        OOPS_ON_ERROR(err);

        err = process_message(m);
        if (err == NO_REPLY) {
            continue;
        }

        if (err != OK) {
            m->header = ERROR_TO_HEADER(err);
        }

        err = kernel_ipc(m->from, IPC_SEND);
        if (err != OK && err != ERR_NEEDS_RETRY) {
            OOPS_ON_ERROR(err);
        }
    }
}

/// The kernel server thread entrypoint.
static void kernel_server_main(void) {
    ASSERT(CURRENT->process == kernel_process);
    OOPS_ON_ERROR(table_init(&user_timers));
    OOPS_ON_ERROR(table_init(&irq_listeners));
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
