#include <arch.h>
#include <debug.h>
#include <list.h>
#include <memory.h>
#include <printk.h>
#include <process.h>
#include <table.h>
#include <thread.h>

/// The PAGE_SIZE-sized memory pool.
struct kmalloc_arena page_arena;
/// The memory pool for small objects.
struct kmalloc_arena small_arena;

static void add_free_list(struct kmalloc_arena *arena, vaddr_t addr,
                          size_t num_objects) {
    struct free_list *free_list = (struct free_list *) addr;
#ifdef DEBUG_BUILD
    asan_init_area(ASAN_UNINITIALIZED, free_list, sizeof(*free_list));
#endif
    free_list->num_objects = num_objects;
    list_push_back(&arena->free_list, &free_list->next);
}

/// Creates a new memory pool.
void arena_init(struct kmalloc_arena *arena, vaddr_t addr, size_t arena_size,
                size_t object_size) {
    arena->object_size = object_size;
    list_init(&arena->free_list);
    add_free_list(arena, addr, arena_size / arena->object_size);
}

/// Allocates a memory. Don't use this directly; use KMALLOC() macro.
void *kmalloc_from(struct kmalloc_arena *arena) {
    if (list_is_empty(&arena->free_list)) {
        PANIC("Run out of kernel memory.");
    }

    struct free_list *free_list = LIST_CONTAINER(
        list_pop_front(&arena->free_list), struct free_list, next);
    ASSERT(free_list->num_objects >= 1);
    free_list->num_objects--;
    if (free_list->num_objects > 0) {
        list_push_back(&arena->free_list, &free_list->next);
    }

    vaddr_t allocated =
        (vaddr_t) free_list + free_list->num_objects * arena->object_size;
#ifdef DEBUG_BUILD
    asan_init_area(ASAN_UNINITIALIZED, (void *) allocated, arena->object_size);
#endif
    return (void *) allocated;
}

/// Frees a memory.
void kfree(struct kmalloc_arena *arena, void *ptr) {
#ifdef DEBUG_BUILD
    asan_init_area(ASAN_FREED, (void *) ptr, arena->object_size);
#endif
    add_free_list(arena, (vaddr_t) ptr, 1);
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
