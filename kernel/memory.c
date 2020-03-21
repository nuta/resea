#include "memory.h"
#include <arch.h>
#include <message.h>
#include <string.h>
#include "ipc.h"
#include "printk.h"
#include "syscall.h"
#include "task.h"

extern char __kernel_heap[];
extern char __kernel_heap_end[];
extern char __initfs[];
extern char __temp_page[];

static list_t heap;

static void add_free_list(void *addr, size_t num_pages) {
    struct free_list *free = addr;
    free->num_pages = num_pages;
    free->magic1 = FREE_LIST_MAGIC1;
    free->magic2 = FREE_LIST_MAGIC2;
    list_push_back(&heap, &free->next);
}

/// Allocates a memory space in the kernel heap. The address is always aligned
/// to PAGE_SIZE.
void *kmalloc(size_t size) {
    if (list_is_empty(&heap)) {
        PANIC("Run out of kernel memory.");
    }

    struct free_list *free =
        LIST_CONTAINER(list_pop_front(&heap), struct free_list, next);

    ASSERT(size <= PAGE_SIZE);
    ASSERT(free->magic1 == FREE_LIST_MAGIC1);
    ASSERT(free->magic2 == FREE_LIST_MAGIC2);
    ASSERT(free->num_pages >= 1);

    free->num_pages--;
    if (free->num_pages > 0) {
        list_push_back(&heap, &free->next);
    }

    void *ptr = (void *) ((vaddr_t) free + free->num_pages * PAGE_SIZE);
    return ptr;
}

/// Frees a memory.
void kfree(void *ptr) {
    add_free_list(ptr, 1);
}

/// Calls the pager task. It always returns a valid paddr: if the memory access
/// is invalid, the pager kills the task instead of replying the page fault
/// message.
static paddr_t user_pager(vaddr_t addr, pagefault_t fault, pageattrs_t *attrs) {
    struct message m;
    m.type = PAGE_FAULT_MSG;
    m.page_fault.task = CURRENT->tid;
    m.page_fault.vaddr = addr;
    m.page_fault.fault = fault;

    error_t err = ipc(CURRENT->pager, CURRENT->pager->tid, &m,
                      IPC_CALL | IPC_KERNEL);
    if (IS_ERROR(err)) {
        WARN("%s: aborted kernel ipc", CURRENT->name);
        task_exit(EXP_ABORTED_KERNEL_IPC);
    }

    // Check if the reply is valid.
    if (m.type != PAGE_FAULT_REPLY_MSG) {
        WARN("%s: invalid page fault reply (type=%d, vaddr=%p, pager=%s)",
             CURRENT->name, m.type, addr, CURRENT->pager->name);
        task_exit(EXP_INVALID_PAGE_FAULT_REPLY);
    }

    // Check that paddr is not in kernel area.
    // TODO: Replace with vm_resolve(m.page_fault_reply.vaddr) in case the pager
    // is malicious.
    paddr_t paddr = m.page_fault_reply.paddr;
    if (is_kernel_paddr(paddr)) {
        WARN("%s: pager returned a kernel page (vaddr=%p, paddr=%p, pager=%s)",
             CURRENT->name, addr, paddr, CURRENT->pager->name);
        task_exit(EXP_INVALID_PAGE_FAULT_REPLY);
    }

    *attrs = PAGE_USER | m.page_fault_reply.attrs;
    return paddr;
}

/// Handles page faults in the initial task.
static paddr_t init_task_pager(vaddr_t vaddr, pageattrs_t *attrs) {
    paddr_t paddr;
    if (INITFS_ADDR <= vaddr && vaddr < INITFS_END) {
        // Initfs contents.
        paddr = into_paddr(__initfs + (vaddr - INITFS_ADDR));
    } else if (STRAIGHT_MAP_ADDR <= vaddr && vaddr < STRAIGHT_MAP_END) {
        // Straight-mapping: virtual addresses are equal to physical.
        paddr = vaddr;
    } else {
        PANIC("init_task tried to access invalid memory address %p", vaddr);
    }

    *attrs = PAGE_USER | PAGE_WRITABLE;
    return paddr;
}

/// The page fault handler. It calls a pager and updates the page table.
paddr_t handle_page_fault(vaddr_t addr, pagefault_t fault) {
    if (is_kernel_addr_range(addr, 0) || addr == (vaddr_t) __temp_page) {
        // The user is not allowed to access the page.
        task_exit(EXP_INVALID_MEMORY_ACCESS);
    }

    // Ask the associated pager to resolve the page fault.
    vaddr_t aligned_vaddr = ALIGN_DOWN(addr, PAGE_SIZE);
    paddr_t paddr;
    pageattrs_t attrs;
    if (CURRENT->tid == INIT_TASK_TID) {
        paddr = init_task_pager(aligned_vaddr, &attrs);
    } else {
        paddr = user_pager(aligned_vaddr, fault, &attrs);
    }

    vm_link(&CURRENT->vm, aligned_vaddr, paddr, attrs);
    return paddr;
}

/// Initializes the memory subsystem.
void memory_init(void) {
    size_t heap_size = (vaddr_t) __kernel_heap_end - (vaddr_t) __kernel_heap;
    INFO("kernel heap: %p - %p (%dKiB)", (vaddr_t) __kernel_heap,
         (vaddr_t) __kernel_heap_end, heap_size / 1024);
    list_init(&heap);
    add_free_list((void *) __kernel_heap, heap_size / PAGE_SIZE);
}
