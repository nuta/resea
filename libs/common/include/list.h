#ifndef __LIST_H__
#define __LIST_H__

#include <types.h>

#define LIST_INIT(list) { .prev = &(list), .next = &(list) }

#define LIST_POP_FRONT(list, container, field)                                 \
    ({                                                                         \
        list_elem_t *__elem = list_pop_front(list);                            \
        (__elem) ? LIST_CONTAINER(__elem, container, field) : NULL;            \
    })

#define LIST_CONTAINER(head, container, field)                                 \
    ((container *) ((vaddr_t)(head) -offsetof(container, field)))

//  Usage:
//
//    struct element {
//        struct list_elem next;
//        int foo;
//    };
//
//    LIST_FOR_EACH(elem, &elems, struct element, next) {
//        printf("foo: %d", elem->foo);
//    }
//
#define LIST_FOR_EACH(elem, list, container, field)                            \
    for (container *elem = LIST_CONTAINER((list)->next, container, field),     \
                   *__next = NULL;                                             \
         (&elem->field != (list)                                               \
          && (__next = LIST_CONTAINER(elem->field.next, container, field)));   \
         elem = __next)

struct list_elem {
    struct list_elem *prev;
    struct list_elem *next;
};

typedef struct list_elem list_t;
typedef struct list_elem list_elem_t;

bool list_is_empty(list_t *list);
size_t list_len(list_t *list);
bool list_contains(list_t *list, list_elem_t *elem);
void list_remove(list_elem_t *elem);
void list_push_back(list_t *list, list_elem_t *new_tail);
list_elem_t *list_pop_front(list_t *list);

#endif
