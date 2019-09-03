#include <arch.h>
#include <list.h>
#include <memory.h>
#include <printk.h>
#include <process.h>
#include <table.h>
#include <thread.h>

/// Memory pool.
struct arena {
    spinlock_t lock;
    /// The memory pool buffer.
    vaddr_t elements;
    /// The size of the buffer.
    size_t element_size;
    /// The miximum number of elements.
    size_t elements_max;
    /// The number of allocated (in use) elements.
    size_t elements_used;
    /// The list of unused elements.
    struct list_head free_list;
};

/// Unused element.
struct freed_element {
    struct list_head next;
};

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
    spin_lock_init(&arena->lock);
    list_init(&arena->free_list);
}

/// Allocates a memory.
void *kmalloc(struct arena *arena) {
    flags_t flags = spin_lock_irqsave(&arena->lock);

    if (arena->elements_used < arena->elements_max) {
        // Allocate from the uninitialized memory space.
        size_t index = arena->elements_used;
        void *ptr = (void *) (arena->elements + arena->element_size * index);
        arena->elements_used++;
        spin_unlock_irqrestore(&arena->lock, flags);
        return ptr;
    }

    spin_unlock_irqrestore(&arena->lock, flags);
    return list_pop_front(&arena->free_list);
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
    TRACE("page_fault_handler: addr=%p", addr);

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
    LIST_FOR_EACH(entry, &process->vmareas) {
        struct vmarea *vma = LIST_CONTAINER(vmarea, next, entry);
        if (is_valid_page_fault_for(vma, aligned_vaddr, flags)) {
            // Ask the associated pager to fill a physical page.
            TRACE(
                "calling pager: pager=%p, vaddr=%p", vma->pager, aligned_vaddr);
            paddr_t paddr = vma->pager(vma, aligned_vaddr);

            if (!paddr) {
                TRACE("failed to fill a page");
                thread_kill_current();
            }

            // Register the filled page with the page table.
            TRACE("#PF: link vaddr %p to %p", aligned_vaddr, paddr);
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

void memory_init(void) {
    arena_init(&small_arena, SMALL_ARENA_ADDR, SMALL_ARENA_LEN, OBJ_MAX_SIZE);
    arena_init(&page_arena, PAGE_ARENA_ADDR, PAGE_ARENA_LEN, PAGE_SIZE);
}
