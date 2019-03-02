#include <arch.h>
#include <thread.h>
#include <printk.h>
#include <memory.h>
#include <collections.h>

struct pool {
    spinlock_t lock;
    vaddr_t elements;
    size_t element_size;
    size_t elements_max;
    size_t elements_used;
    struct list_head free_list;
};

struct free_element {
    struct list_head next;
};

static struct pool page_pool;
static struct pool obj_pool;

void pool_init(struct pool *pool, vaddr_t addr, size_t pool_size, size_t elem_size) {
    pool->elements = addr;
    pool->element_size = elem_size;
    pool->elements_max = pool_size / elem_size;
    pool->elements_used = 0;
    spin_lock_init(&pool->lock);
    list_init(&pool->free_list);
}

void *pool_alloc(struct pool *pool) {
    flags_t flags = spin_lock_irqsave(&pool->lock);

    if (pool->elements_used < pool->elements_max) {
        // Allocate from the uninitialized memory space.
        size_t index = pool->elements_used;
        void *ptr = (void *) (pool->elements + pool->element_size * index);
        pool->elements_used++;
        spin_unlock_irqrestore(&pool->lock, flags);
        return ptr;
    }

    spin_unlock_irqrestore(&pool->lock, flags);
    return list_pop_front(&pool->free_list);
}

void *alloc_page(void) {
    return pool_alloc(&page_pool);
}

void *alloc_object(void) {
    return pool_alloc(&obj_pool);
}

void free_object(UNUSED void *ptr) {
    // TODO:
}

void free_page(UNUSED void *ptr) {
    // TODO:
}

void page_fault_handler(vaddr_t addr, uintmax_t flags) {
    DEBUG("page_fault_handler: addr=%p", addr);

    if (!(flags & PF_USER)) {
        // This will never occur. NEVER!
        PANIC("page fault in the kernel space");
    }

    if (flags & PF_PRESENT) {
        // Invalid access. For instance the user thread has tried to write to
        // readonly area.
        DEBUG("page fault: already present: addr=%p", addr);
        thread_kill_current();
    }


    // Look for the associated vm area.
    vaddr_t aligned_vaddr = ALIGN_DOWN(addr, PAGE_SIZE);
    struct process *process = CURRENT->process;
    LIST_FOR_EACH(entry, &process->vmareas) {
        struct vmarea *vma = LIST_CONTAINER(vmarea, next, entry);
        if (!(vma->start <= aligned_vaddr && aligned_vaddr < vma->end)) {
            continue;
        }

        if (flags & PF_USER && vma->flags & !(PAGE_USER)) {
            continue;
        }

        if (flags & PF_WRITE && vma->flags & !(PAGE_WRITABLE)) {
            continue;
        }

        // This area includes `addr`, allows the requested access, and
        // the page does not yet exist in the page table. Looks a valid
        // page fault. Let's handle this!

        // Ask the associated pager to fill a physical page.
        DEBUG("calling pager: pager=%p, vaddr=%p", vma->pager, aligned_vaddr);
        paddr_t paddr = vma->pager(vma, aligned_vaddr);

        if (!paddr) {
            DEBUG("failed to fill a page");
            thread_kill_current();
        }

        // Register the filled page with the page table.
        DEBUG("#PF: link vaddr %p to %p", aligned_vaddr, paddr);
        arch_link_page(&process->page_table, aligned_vaddr, paddr, 1, vma->flags);

        // Now we've done what we have to do. Return to the exception
        // handler and resume the thread.
        return;
    }

    // No appropriate vm area. The user thread must have accessed unallocated
    // area (e.g. NULL pointer deference).
    PANIC("page fault: no appropriate vmarea: addr=%p", addr);
    thread_kill_current();
}

void memory_init(void) {
    pool_init(&obj_pool, OBJ_POOL_ADDR, OBJ_POOL_LEN, OBJ_MAX_SIZE);
    pool_init(&page_pool, PAGE_POOL_ADDR, PAGE_POOL_LEN, PAGE_SIZE);

}
