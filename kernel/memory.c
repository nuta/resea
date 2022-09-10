#include "memory.h"
#include "arch.h"
#include "ipc.h"
#include "printk.h"
#include "task.h"
#include <string.h>

static list_t zones = LIST_INIT(zones);

static bool is_contiguously_free(struct memory_zone *zone, size_t start,
                                 size_t num_pages) {
    for (size_t i = start; i < zone->num_pages; i++) {
        if (zone->pages[i].ref_count != 0) {
            return false;
        }
    }
    return true;
}

static struct page *find_page_by_paddr(paddr_t paddr) {
    LIST_FOR_EACH(zone, &zones, struct memory_zone, next) {
        if (zone->base <= paddr
            && paddr < zone->base + zone->num_pages * PAGE_SIZE) {
            size_t start = (paddr - zone->base) / PAGE_SIZE;
            return &zone->pages[start];
        }
    }

    return NULL;
}

static void add_zone(paddr_t paddr, size_t size) {
    struct memory_zone *zone = (struct memory_zone *) arch_paddr2ptr(paddr);
    // TODO: Is this correct?
    size_t num_pages = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;

    void *end_of_header = &zone->pages[num_pages + 1];
    size_t header_size = ((vaddr_t) end_of_header) - ((vaddr_t) zone);

    zone->base = paddr + ALIGN_UP(header_size, PAGE_SIZE);
    zone->num_pages = num_pages;
    for (size_t i = 0; i < num_pages; i++) {
        zone->pages[i].ref_count = 0;
    }

    list_elem_init(&zone->next);
    list_push_back(&zones, &zone->next);
}

paddr_t pm_alloc(size_t size, unsigned type, unsigned flags) {
    size_t aligned_size = ALIGN_UP(size, PAGE_SIZE);
    size_t num_pages = aligned_size / PAGE_SIZE;
    LIST_FOR_EACH(zone, &zones, struct memory_zone, next) {
        for (size_t start = 0; start < zone->num_pages; start++) {
            paddr_t paddr = zone->base + start * PAGE_SIZE;
            if ((flags & PM_ALLOC_ALIGNED) != 0
                && !IS_ALIGNED(paddr, aligned_size)) {
                continue;
            }

            if (is_contiguously_free(zone, start, num_pages)) {
                for (size_t i = 0; i < num_pages; i++) {
                    zone->pages[start + i].ref_count = 1;
                    zone->pages[start + i].type = type;
                }

                if (flags & PM_ALLOC_UNINITIALIZED) {
                    memset(arch_paddr2ptr(paddr), 0, PAGE_SIZE * num_pages);
                }

                return paddr;
            }
        }
    }

    PANIC("no memory");
    return 0;
}

void pm_free(paddr_t paddr, size_t size) {
    struct page *page = find_page_by_paddr(paddr);
    ASSERT(page != NULL);

    size_t aligned_size = ALIGN_UP(size, PAGE_SIZE);
    size_t num_pages = aligned_size / PAGE_SIZE;

    for (size_t i = 0; i < num_pages; i++) {
        DEBUG_ASSERT(page->ref_count > 0);
        page->ref_count--;
    }
}

error_t vm_map(struct task *task, uaddr_t uaddr, paddr_t paddr, size_t size,
               unsigned attrs) {
    // FIXME: Deny kernel addresses.
    return arch_vm_map(&task->vm, uaddr, paddr, size, attrs);
}

/// The page fault handler. It calls a pager to ask to update the page table.
void handle_page_fault(vaddr_t vaddr, vaddr_t ip, unsigned fault) {
    if ((fault & PAGE_FAULT_USER) == 0) {
        for (;;)
            __asm__ __volatile__("wfi");
        PANIC("page fault in kernel space: vaddr=%p, ip=%p, reason=%x", vaddr,
              ip, fault);
    }

    struct task *pager = CURRENT_TASK->pager;
    if (!pager) {
        PANIC("page fault in the init task: vaddr=%p, ip=%p", vaddr, ip);
    }

    // TODO: Check if vaddr is kernel

    struct message m;
    m.type = PAGE_FAULT_MSG;
    m.page_fault.task = CURRENT_TASK->tid;
    m.page_fault.uaddr = vaddr;
    m.page_fault.ip = ip;
    m.page_fault.fault = fault;
    error_t err = ipc(pager, pager->tid, (__user struct message *) &m,
                      IPC_CALL | IPC_KERNEL);
    if (err != OK || m.type != PAGE_FAULT_REPLY_MSG) {
        // TODO:
        UNREACHABLE();
    }
}

void memory_init(struct bootinfo *bootinfo) {
    struct memory_map *memory_map = &bootinfo->memory_map;
    for (int i = 0; i < memory_map->num_entries; i++) {
        add_zone(memory_map->entries[i].paddr, memory_map->entries[i].size);
    }
}
