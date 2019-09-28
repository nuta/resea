#include <arch.h>
#include <debug.h>
#include <list.h>
#include <memory.h>
#include <printk.h>
#include <process.h>
#include <table.h>
#include <thread.h>

/// The PAGE_SIZE-sized memory pool.
struct arena page_arena;
/// The memory pool for small objects.
struct arena small_arena;

/// Creates a new memory pool.
void arena_init(struct arena *arena, vaddr_t addr, size_t arena_size,
                size_t elem_size) {
    arena->elements = addr;
    arena->element_size = elem_size;
    arena->elements_max = arena_size / elem_size;
    arena->elements_used = 0;
    list_init(&arena->free_list);
}

/// Allocates a memory. Don't use this directly; use KMALLOC() macro.
void *kmalloc_from(struct arena *arena) {
    if (arena->elements_used < arena->elements_max) {
        // Allocate from the uninitialized memory space.
        size_t index = arena->elements_used;
        void *ptr = (void *) (arena->elements + arena->element_size * index);
        arena->elements_used++;

#ifdef DEBUG_BUILD
        asan_init_area(ASAN_UNINITIALIZED, ptr, arena->element_size);
#endif
        return ptr;
    }

    if (list_is_empty(&arena->free_list)) {
        PANIC("Run out of kernel memory.");
    }

    void *ptr = list_pop_front(&arena->free_list);
#ifdef DEBUG_BUILD
    asan_init_area(ASAN_UNINITIALIZED, ptr, arena->element_size);
#endif
    return ptr;
}

/// Frees a memory.
void kfree(UNUSED struct arena *arena, UNUSED void *ptr) {
    // TODO:
    UNIMPLEMENTED();
}

/// Checks if `vma` includes `addr` and allows the requested access.
static int is_valid_page_fault_for(
    struct vmarea *vma, vaddr_t vaddr, uintmax_t flags) {
    if (!(vma->start <= vaddr && vaddr < vma->end)) {
        return 0;
    }

    if (flags & PF_USER && vma->flags & !(PAGE_USER)) {
        return 0;
    }

    if (flags & PF_WRITE && vma->flags & !(PAGE_WRITABLE)) {
        return 0;
    }

    return 1;
}

/// The page fault handler. It calls pagers and updates the page table. If the
/// page fault is invalid (e.g., segfault and NULL pointer dereference), kill
/// the current thread.
paddr_t page_fault_handler(vaddr_t addr, uintmax_t flags) {
    // TRACE("page_fault_handler: addr=%p", addr);

    if (!(flags & PF_USER)) {
        // This will never occur. NEVER!
        PANIC("page fault in the kernel space");
    }

    if (flags & PF_PRESENT) {
        // Invalid access. For instance the user thread has tried to write to
        // readonly area.
        TRACE("page fault: already present: addr=%p", addr);
        thread_kill_current();
    }

    // Look for the associated vm area.
    vaddr_t aligned_vaddr = ALIGN_DOWN(addr, PAGE_SIZE);
    struct process *process = CURRENT->process;
    LIST_FOR_EACH(vma, &process->vmareas, struct vmarea, next) {
        if (is_valid_page_fault_for(vma, aligned_vaddr, flags)) {
            // Ask the associated pager to fill a physical page.
            // TRACE(
            //     "calling pager: pager=%p, vaddr=%p", vma->pager, aligned_vaddr);
            paddr_t paddr = vma->pager(vma, aligned_vaddr);

            if (!paddr) {
                TRACE("failed to fill a page");
                thread_kill_current();
            }

            // Register the filled page with the page table.
            // TRACE("#PF: link vaddr %p to %p", aligned_vaddr, paddr);
            link_page(
                &process->page_table, aligned_vaddr, paddr, 1, vma->flags);

            // Now we've done what we have to do. Return to the exception
            // handler and resume the thread.
            return paddr;
        }
    }

    // No appropriate vm area. The user thread must have accessed unallocated
    // area (e.g. NULL pointer dereference).
    PANIC("page fault: no appropriate vmarea: addr=%p", addr);
    thread_kill_current();
}

/// Initializes the memory subsystem.
void memory_init(void) {
    arena_init(&small_arena, SMALL_ARENA_ADDR, SMALL_ARENA_LEN, OBJ_MAX_SIZE);
    arena_init(&page_arena, PAGE_ARENA_ADDR, PAGE_ARENA_LEN, PAGE_SIZE);
}
