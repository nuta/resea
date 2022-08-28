#include <list.h>

bool list_is_empty(list_t *list) {
    return list->next == list;
}

size_t list_len(list_t *list) {
    size_t len = 0;
    struct list_head *node = list->next;
    while (node != list) {
        len++;
        node = node->next;
    }

    return len;
}

bool list_contains(list_t *list, list_elem_t *elem) {
    list_elem_t *node = list->next;
    while (node != list) {
        if (node == elem) {
            return true;
        }
        node = node->next;
    }

    return false;
}

// Inserts a new element between `prev` and `next`.
void list_insert(list_elem_t *prev, list_elem_t *next,
                               list_elem_t *new) {
    new->prev = prev;
    new->next = next;
    next->prev = new;
    prev->next = new;
}

// Initializes a list.
void list_init(list_t *list) {
    list->prev = list;
    list->next = list;
}

// Invalidates a list element.
void list_nullify(list_elem_t *elem) {
    elem->prev = NULL;
    elem->next = NULL;
}

// Removes a element from the list.
void list_remove(list_elem_t *elem) {
    if (!elem->next) {
        // The element is not in a list.
        return;
    }

    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;

    // Invalidate the element as they're no longer in the list.
    list_nullify(elem);
}

// Appends a element into the list.
void list_push_back(list_t *list, list_elem_t *new_tail) {
    DEBUG_ASSERT(!list_contains(list, new_tail));
    list_insert(list->prev, list, new_tail);
}

// Get and removes the first element from the list.
list_t *list_pop_front(list_t *list) {
    struct list_head *head = list->next;
    if (head == list) {
        return NULL;
    }

    // list <-> head <-> next => list <-> next
    struct list_head *next = head->next;
    list->next = next;
    next->prev = list;

    // Invalidate the element as they're no longer in the list.
    list_nullify(head);
    return head;
}
