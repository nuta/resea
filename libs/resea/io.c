#include <resea/io.h>
#include <resea/ipc.h>
#include <resea/task.h>
#include <resea/printf.h>
#include <resea/syscall.h>

error_t irq_acquire(unsigned irq) {
    return sys_listen(-1 /* do nothing */, irq + 1);
}

error_t irq_release(unsigned irq) {
    return sys_listen(-1 /* do nothing */, -(irq + 1));
}

void *io_alloc_pages(size_t num_pages, paddr_t map_to, paddr_t *paddr) {
    task_t init = 1;

    // Request alloc_pages.
    struct message m;
    m.type = ALLOC_PAGES_MSG;
    m.alloc_pages.paddr = map_to;
    m.alloc_pages.num_pages = num_pages;
    error_t err = ipc_call(init, &m);
    ASSERT_OK(err);
    ASSERT(m.type == ALLOC_PAGES_REPLY_MSG);

    *paddr = m.alloc_pages_reply.paddr;
    return (void *) m.alloc_pages_reply.vaddr;
}
