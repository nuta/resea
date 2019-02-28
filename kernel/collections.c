#include <collections.h>
#include <printk.h>

// Inserts a new element between `prev` and `next`.
static void list_insert(struct list_head *prev,
                        struct list_head *next,
                        struct list_head *new) {
    new->prev = prev;
    new->next = next;
    next->prev = new;
    prev->next = new;
}

// Initializes a list.
void list_init(struct list_head *list) {
    list->prev = list;
    list->next = list;
}

// Appends a element into the list.
void list_push_back(struct list_head *list, struct list_head *new_tail) {
    list_insert(list->prev, list, new_tail);
}

// Get and removes the first element from the list.
struct list_head *list_pop_front(struct list_head *list) {
    struct list_head *head = list->next;
    if (head == list) {
        return NULL;
    }

    // list <-> head <-> next => list <-> next
    struct list_head *next = head->next;
    list->next = next;
    next->prev = list;
    return head;
}

int list_is_empty(struct list_head *list) {
    return list->next == list;
}

static int id_to_index(int id) {
    if (id < 1) {
        BUG("tried to use an invalid id (id=%d)", id);
    }

    return id - 1;
}

// Returns 0 on failure.
int idtable_init(struct idtable *table) {
    void **entries = alloc_page();
    if (!entries) {
        return 0;
    }

    size_t num = PAGE_SIZE / sizeof(void *);
    for (int i = 0; i < table->num; i++) {
        entries[i] = NULL;
    }

    table->entries = entries;
    table->num = num;
    return 1;
}

// You don't have to use spinlock for this. The implementation is lock-free.
int idtable_alloc(struct idtable *table) {
    for (int i = 0; i < table->num; i++) {
        if (atomic_compare_and_swap(&table->entries[i], NULL, RESERVED)) {
            // Successfully reserved the entry.
            return i + 1;
        }
    }

    return 0;
}

// You don't have to use spinlock for this. The implementation is lock-free.
void idtable_free(struct idtable *table, int id) {
    int index = id_to_index(id);
    __atomic_store_n(&table->entries[index], NULL, __ATOMIC_SEQ_CST);
}

// You don't have to use spinlock for this. The implementation is lock-free.
void idtable_set(struct idtable *table, int id, void *value) {
    int index = id_to_index(id);

    if (table->entries[index] != RESERVED) {
        BUG("tried to use an allocated id, (id=%d)", id);
    }

    __atomic_store_n(&table->entries[index], value, __ATOMIC_SEQ_CST);
}

// You don't have to use spinlock for this. The implementation is lock-free.
void *idtable_get(struct idtable *table, int id) {
    int index = id_to_index(id);
    return __atomic_load_n(&table->entries[index], __ATOMIC_SEQ_CST);
}
