#include <ipc.h>
#include <process.h>
#include <server.h>
#include <thread.h>
#include <printk.h>

static paddr_t user_pager(struct vmarea *vma, vaddr_t vaddr) {
    struct channel *pager = vma->arg;
    DEBUG("user pager=%d, addr=%p", pager->cid, vaddr);

    // FIXME: define message structs
    struct message *m = CURRENT->buffer;
    m->header = FILL_PAGE_REQUEST << MSG_ID_OFFSET
              | sizeof(*m) << INLINE_PAYLOAD_LEN_OFFSET;
    m->inlines[0] = CURRENT->process->pid;
    m->inlines[1] = vaddr;

    payload_t options = 1 << FLAG_SEND_OFFSET | 1 << FLAG_RECV_OFFSET;
    sys_ipc(pager->cid, options);

    paddr_t paddr = m->pages[0];
    DEBUG("Ok, got a page addr=%p", paddr);
    return paddr;
}

error_t kernel_server(void) {
    struct message *m = CURRENT->buffer;
    switch (MSG_ID(m->header)) {
        case PUTCHAR_REQUEST: {
            arch_putchar(m->inlines[0]);
            m->header = PUTCHAR_RESPONSE;
            break;
        }
        case CREATE_PROCESS_REQUEST: {
            DEBUG("kernel: create_process()");
            struct process *proc = process_create("user" /* TODO: */);
            if (!proc) {
                return ERR_OUT_OF_RESOURCE;
            }

            struct channel *user_ch = channel_create(proc);
            if (!user_ch) {
                process_destroy(proc);
                return ERR_OUT_OF_RESOURCE;
            }

            struct channel *pager_ch = channel_create(CURRENT->process);
            if (!pager_ch) {
                channel_destroy(pager_ch);
                process_destroy(proc);
                return ERR_OUT_OF_RESOURCE;
            }

            channel_link(user_ch, pager_ch);

            DEBUG("kernel: create_process_response(pid=%d, pager_ch=%d)",
                proc->pid, pager_ch->cid);
            m->header = CREATE_PROCESS_RESPONSE;
            m->inlines[0] = proc->pid;
            m->inlines[1] = pager_ch->cid;
            break;
        }
        case SPAWN_THREAD_REQUEST: {
            int pid = m->inlines[0];
            vaddr_t start = m->inlines[1];
            vaddr_t stack = m->inlines[2];
            vaddr_t buffer = m->inlines[3];
            uintmax_t arg = m->inlines[4];
            DEBUG("kernel: spawn_thread(pid=%d, start=%p)", pid, start);

            struct process *proc = idtable_get(&all_processes, pid);
            if (!proc) {
                return ERR_INVALID_MESSAGE;
            }

            struct thread *thread = thread_create(proc, start, stack, buffer, arg);
            if (!thread) {
                return ERR_OUT_OF_RESOURCE;
            }

            thread_resume(thread);

            DEBUG("kernel: spawn_thread_response(tid=%d)", thread->tid);
            m->header = SPAWN_THREAD_RESPONSE;
            m->inlines[0] = thread->tid;
            break;
        }
        case ADD_PAGER_REQUEST: {
            int pid = m->inlines[0];
            int pager = m->inlines[1];
            vaddr_t start = m->inlines[2];
            vaddr_t end = m->inlines[3];
            int flags = m->inlines[4];
            flags |= PAGE_USER;

            DEBUG("kernel: add_pager(pid=%d, pager=%d, range=%p-%p)",
                pid, pager, start, end);

            struct process *proc = idtable_get(&all_processes, pid);
            if (!proc) {
                return ERR_INVALID_MESSAGE;
            }

            struct channel *pager_ch = idtable_get(&proc->channels, pager);
            if (!pager_ch) {
                return ERR_INVALID_MESSAGE;
            }

            vmarea_add(proc, start, end, user_pager, pager_ch, flags);

            DEBUG("kernel: add_pager_response()");
            m->header = ADD_PAGER_RESPONSE;
            break;
        }
        case ALLOW_IO_REQUEST: {
            DEBUG("kernel: allow_io()");

            // Allow *ALL* io ports for now.
            // TODO: Allow only specific ports by updating the TSS io port bitmap on
            //       context switches.
            arch_allow_io(CURRENT);
            m->header = ALLOW_IO_RESPONSE;
            break;
        }
        case CREATE_KERNEL_CHANNEL_REQUEST: {
            intmax_t pid = m->inlines[0];
            DEBUG("kernel: create_kernel_channel(pid=%d)", pid);

            struct process *proc = idtable_get(&all_processes, pid);
            if (!proc) {
                return ERR_INVALID_MESSAGE;
            }

            struct channel *their_ch = channel_create(proc);
            if (!their_ch) {
                return ERR_OUT_OF_RESOURCE;
            }

            struct channel *our_ch = channel_create(kernel_process);
            if (!our_ch) {
                channel_destroy(their_ch);
                return ERR_OUT_OF_RESOURCE;
            }

            channel_link(their_ch, our_ch);
            m->header = CREATE_KERNEL_CHANNEL_RESPONSE;
            m->inlines[0] = their_ch->cid;
            break;
        }
        case CREATE_THREAD_REQUEST: {
            int pid = m->inlines[0];
            vaddr_t start = m->inlines[1];
            vaddr_t stack = m->inlines[2];
            vaddr_t buffer = m->inlines[3];
            uintmax_t arg = m->inlines[4];
            DEBUG("kernel: create_thread(pid=%d, start=%p)", pid, start);

            struct process *proc = idtable_get(&all_processes, pid);
            if (!proc) {
                return ERR_INVALID_MESSAGE;
            }

            struct thread *thread = thread_create(proc, start, stack, buffer, arg);
            if (!thread) {
                return ERR_OUT_OF_RESOURCE;
            }

            DEBUG("kernel: create_thread_response(tid=%d)", thread->tid);
            m->header = CREATE_THREAD_RESPONSE;
            m->inlines[0] = thread->tid;
            break;
        }
        case START_THREAD_REQUEST: {
            int tid = m->inlines[0];
            DEBUG("kernel: start_thread(tid=%d)", tid);

            struct thread *thread = idtable_get(&all_processes, tid);
            if (!thread) {
                return ERR_INVALID_MESSAGE;
            }

            thread_resume(thread);

            DEBUG("kernel: create_thread_response()");
            m->header = START_THREAD_RESPONSE;
            break;
        }
    }

    return OK;
}
