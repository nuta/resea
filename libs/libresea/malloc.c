#include <resea.h>
#include <resea/ipc.h>
#include <resea/math.h>

// TODO: Support freeing pages.
static vaddr_t valloc_next = 0xa0000000;
page_base_t valloc(size_t num_pages) {
    vaddr_t addr = valloc_next;
    valloc_next += num_pages * PAGE_SIZE;
    return PAGE_PAYLOAD(addr, ulllog2(num_pages));
}
