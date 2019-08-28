#ifndef __TABLE_H__
#define __TABLE_H__

#include <memory.h>
#include <printk.h>
#include <types.h>

// We don't allocate id == 0 to use it as a *null* value.
#define ID_START 1
// Indicates that the entry is allocated but not yet set a value.
#define RESERVED ((void *) 0xcafecafe)

struct idtable {
    void **entries;
    int num_entries;
};

// Returns 0 on failure.
static inline int idtable_init(struct idtable *table) {
    void **entries = kmalloc(&page_arena);
    if (!entries) {
        return 0;
    }

    size_t num = PAGE_SIZE / sizeof(void *);
    for (int i = 0; i < table->num_entries; i++) {
        entries[i] = NULL;
    }

    table->entries = entries;
    table->num_entries = num;
    return 1;
}

// You don't have to use spinlock for this. The implementation is lock-free.
static inline int idtable_alloc(struct idtable *table) {
    for (int i = ID_START; i < table->num_entries; i++) {
        if (atomic_compare_and_swap(&table->entries[i], NULL, RESERVED)) {
            // Successfully reserved the entry.
            return i;
        }
    }

    return 0;
}

// You don't have to use spinlock for this. The implementation is lock-free.
static inline void idtable_free(struct idtable *table, int id) {
    __atomic_store_n(&table->entries[id], NULL, __ATOMIC_SEQ_CST);
}

// You don't have to use spinlock for this. The implementation is lock-free.
static inline void idtable_set(struct idtable *table, int id, void *value) {
    if (table->entries[id] != RESERVED) {
        BUG("tried to use an allocated id, (id=%d)", id);
    }

    if (id == 0 || id >= table->num_entries) {
        // We keep `table->entries[0]` always equals NULL to avoid a conditional
        // branching in `idtable_get()`.
        BUG("bad id (id=%d, num_entries=%d)", id, table->num_entries);
    }

    __atomic_store_n(&table->entries[id], value, __ATOMIC_SEQ_CST);
}

// Returns NULL if `id' equals to 0. You don't have to use spinlock for
// this. The implementation is lock-free.
static inline void *idtable_get(struct idtable *table, int id) {
    // We don't have to check if id != 0 since `table->entries[0]` is always
    // NULL.
    if (id >= table->num_entries) {
        return NULL;
    }

    return __atomic_load_n(&table->entries[id], __ATOMIC_SEQ_CST);
}

#endif
