#include <ipc.h>
#include <process.h>
#include <server.h>
#include <thread.h>
#include <printk.h>

static paddr_t user_pager(struct vmarea *vma, vaddr_t vaddr) {
    struct channel *pager = vma->arg;
    DEBUG("user pager=%d, addr=%p", pager->cid, vaddr);

    payload_t header = (pager->cid << 48) | MSG_SEND | MSG_RECV | FILL_PAGE_REQUEST;
    payload_t p0 = CURRENT->process->pid;
    payload_t p1 = vaddr;
    CURRENT->recved_by_kernel = 1;
    ipc_handler(header, p0, p1, 0, 0, 0);
    CURRENT->recved_by_kernel = 0;

    paddr_t paddr = CURRENT->buf[0];
    DEBUG("Ok, got a page addr=%p", paddr);
    return paddr;
}

error_t kernel_server(payload_t header,
                      payload_t p0,
                      payload_t p1,
                      payload_t p2,
                      payload_t p3,
                      UNUSED payload_t p4,
                      struct thread *client) {

    switch (MSG_ID(header)) {
        case PUTCHAR_REQUEST: {
            arch_putchar(p0);
            client->header = PUTCHAR_RESPONSE;
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
            client->header = CREATE_PROCESS_RESPONSE;
            client->buf[0] = proc->pid;
            client->buf[1] = pager_ch->cid;
            break;
        }
        case SPAWN_THREAD_REQUEST: {
            int pid = p0;
            vaddr_t start = p1;
            DEBUG("kernel: spawn_thread(pid=%d, start=%p)", pid, start);

            struct process *proc = idtable_get(&all_processes, pid);
            if (!proc) {
                return ERR_INVALID_MESSAGE;
            }

            struct thread *thread = thread_create(proc, start, 0, 0);
            if (!thread) {
                return ERR_OUT_OF_RESOURCE;
            }

            thread_resume(thread);

            DEBUG("kernel: spawn_thread_response(tid=%d)", thread->tid);
            client->header = SPAWN_THREAD_RESPONSE;
            client->buf[0] = thread->tid;
            break;
        }
        case ADD_PAGER_REQUEST: {
            int pid = p0;
            int pager = p1;
            vaddr_t vaddr = p2;
            size_t len = p3;
            int flags = p4;
            flags |= PAGE_USER;

            DEBUG("kernel: add_pager(pid=%d, pager=%d, vaddr=%p, len=%p)",
                pid, pager, vaddr, len);

            struct process *proc = idtable_get(&all_processes, pid);
            if (!proc) {
                return ERR_INVALID_MESSAGE;
            }

            struct channel *pager_ch = idtable_get(&proc->channels, pager);
            if (!pager_ch) {
                return ERR_INVALID_MESSAGE;
            }

            vmarea_add(proc, vaddr, len, user_pager, pager_ch, flags);

            DEBUG("kernel: add_pager_response()");
            client->header = ADD_PAGER_RESPONSE;
        }
    }

    return OK;
}
