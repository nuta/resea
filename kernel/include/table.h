#ifndef __TABLE_H__
#define __TABLE_H__

#include <memory.h>
#include <printk.h>
#include <types.h>

/// We don't allocate id == 0 to use it as a *null* value.
#define ID_START 1
/// Indicates that the entry is allocated but not yet set a value.
#define RESERVED ((void *) 0xcafecafe)

/// An array with safety equipments. It provides ID allocation (e.g. process
/// ID) and safe (bounds-checked) set/get operations.
struct table {
    /// Entries in the table. Each entry is either NULL or value: if it's NULL,
    /// the slot is not being used.
    void **entries;
    /// The maximum number of entries.
    int num_entries;
};

/// Initializes the table.
static inline error_t table_init(struct table *table) {
    void **entries = kmalloc(&page_arena);
    if (!entries) {
        return ERR_NO_MEMORY;
    }

    size_t num = PAGE_SIZE / sizeof(void *);
    for (size_t i = 0; i < num; i++) {
        entries[i] = NULL;
    }

    table->entries = entries;
    table->num_entries = num;
    return OK;
}

/// Search the table for an unused slot and mark it as reserved. It returnes
/// the id (always greater than 0) for the reserved slot on success or 0 on
/// failure.
///
/// Note that this operation is O(n).
///
static inline int table_alloc(struct table *table) {
    for (int i = ID_START; i < table->num_entries; i++) {
        if (atomic_compare_and_swap(&table->entries[i], NULL, RESERVED)) {
            // Successfully reserved the entry.
            return i;
        }
    }

    return 0;
}

/// Frees a slot.
static inline void table_free(struct table *table, int id) {
    __atomic_store_n(&table->entries[id], NULL, __ATOMIC_SEQ_CST);
}

/// Sets a slot value.
static inline void table_set(struct table *table, int id, void *value) {
    if (table->entries[id] != RESERVED) {
        BUG("tried to use an allocated id, (id=%d)", id);
    }

    if (id == 0 || id >= table->num_entries) {
        // We keep `table->entries[0]` always equals NULL to avoid a conditional
        // branching in `table_get()`.
        BUG("bad id (id=%d, num_entries=%d)", id, table->num_entries);
    }

    __atomic_store_n(&table->entries[id], value, __ATOMIC_SEQ_CST);
}

/// Returns the slot value or NULL if `id' is invalid.
static inline void *table_get(struct table *table, int id) {
    // We don't have to check if id != 0 since `table->entries[0]` is always
    // NULL.
    if (id >= table->num_entries) {
        return NULL;
    }

    return __atomic_load_n(&table->entries[id], __ATOMIC_SEQ_CST);
}

#endif
