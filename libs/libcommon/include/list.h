#ifndef __LIST_H__
#define __LIST_H__

#include <types.h>

#define LIST_CONTAINER(head, container, field) \
    ((container *) ((vaddr_t) (head) - offsetof(container, field)))
#define LIST_FOR_EACH(elem, list, container, field) \
    for (container *elem = LIST_CONTAINER(list, container, field), \
         *__next = LIST_CONTAINER(elem->field.next, container, field); \
         &elem->field != (list); \
         elem = LIST_CONTAINER(__next, container, field), \
         __next = LIST_CONTAINER(elem->field.next, container, field))

struct list_head {
    struct list_head *prev;
    struct list_head *next;
};

static inline bool list_is_empty(struct list_head *list) {
    return list->next == list;
}

// Inserts a new element between `prev` and `next`.
static inline void list_insert(struct list_head *prev, struct list_head *next,
                               struct list_head *new) {
    new->prev = prev;
    new->next = next;
    next->prev = new;
    prev->next = new;
}


// Initializes a list.
static inline void list_init(struct list_head *list) {
    list->prev = list;
    list->next = list;
}

// Removes a element from the list.
static inline void list_remove(struct list_head *to_remove) {
    to_remove->prev->next = to_remove->next;
    to_remove->next->prev = to_remove->prev;
    to_remove->next = INVALID_POINTER;
    to_remove->prev = INVALID_POINTER;
}

// Appends a element into the list.
static inline void list_push_back(struct list_head *list,
                                  struct list_head *new_tail) {
    list_insert(list->prev, list, new_tail);
}

// Get and removes the first element from the list.
static inline struct list_head *list_pop_front(struct list_head *list) {
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

#endif
