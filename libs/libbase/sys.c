#include <resea.h>
#include <resea_idl.h>
#include <base/sys.h>

// FIXME:
#define HEAP_AREA_START   0x70000000
#define HEAP_AREA_END     0xa0000000
#define VALLOC_AREA_START 0xa0000000
#define VALLOC_AREA_END   0xc0000000

static uintptr_t heap_area_bottom = HEAP_AREA_START;
static uintptr_t grow_heap_area(int order) {
    uintptr_t addr = heap_area_bottom;
    heap_area_bottom += POW2(order) * PAGE_SIZE;
    assert(heap_area_bottom < HEAP_AREA_END);
    return addr;
}

static uintptr_t valloc_area_bottom = 0xa0000000;
page_base_t valloc2(int order) {
    vaddr_t addr = valloc_area_bottom;
    valloc_area_bottom += POW2(order) * PAGE_SIZE;
    assert(valloc_area_bottom < VALLOC_AREA_END);
    return PAGE_PAYLOAD(addr, order, 0);
}

void *sys_alloc_pages(int order) {
    cid_t memmgr_ch = 1;
    page_base_t page_base = PAGE_PAYLOAD(grow_heap_area(order), order, 0);
    size_t num_pages;
    uintptr_t page;
    error_t err = alloc_pages(memmgr_ch, 0, page_base, &page, &num_pages);
    assert(err == OK);
    return (void *) page;
}
