#include <message.h>
#include <std/io.h>
#include <std/lookup.h>
#include <std/printf.h>
#include <std/syscall.h>

void *io_alloc_pages(size_t num_pages, paddr_t map_to, paddr_t *paddr) {
    tid_t init = 1;

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
