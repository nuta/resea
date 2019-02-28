#ifndef __LIST_H__
#define __LIST_H__

#include <types.h>

// Indicates that the entry is allocated but not yet set a value.
#define RESERVED ((void *) 0xcafecafe)

#define LIST_CONTAINER(container, field, node) \
    ((struct container *) ((vaddr_t) (node) - offsetof(struct container, field)))
#define LIST_FOR_EACH(e, list) \
    for (struct list_head *e = (list)->next; e != (list); e = e->next)

struct list_head {
    struct list_head *prev;
    struct list_head *next;
};

struct idtable {
    void **entries;
    int num;
};

void list_init(struct list_head *list);
int list_is_empty(struct list_head *list);
void list_push_back(struct list_head *list, struct list_head *new_tail);
struct list_head *list_pop_front(struct list_head *list);

int idtable_init(struct idtable *table);
int idtable_alloc(struct idtable *table);
void idtable_free(struct idtable *table, int id);
void idtable_set(struct idtable *table, int id, void *value);
void *idtable_get(struct idtable *table, int id);

#endif
