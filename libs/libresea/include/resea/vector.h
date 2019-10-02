#ifndef __BASE_VECTOR_H__
#define __BASE_VECTOR_H__

#include <resea.h>

struct vector {
    /// The elements.
    opaque_t *buf;
    /// The number of elements in `buf`.
    int size;
    /// The capacity of the `buf`.
    int capacity;
    trait_t trait;
};

typedef struct vector *vector_t;

vector_t vector_new(trait_t trait);
void vector_delete(vector_t vec);
int vector_size(vector_t vec);
bool vector_is_empty(vector_t vec);
opaque_t vector_get(vector_t vec, int index);
void vector_set(vector_t vec, int index, opaque_t data);
void vector_resize(vector_t vec, int new_capacity);
void vector_append(vector_t vec, opaque_t data);
opaque_t vector_pop(vector_t vec);

#define vector_foreach_index(vec, index) \
    for (int index = 0; index < (vec)->size; index++)

#endif
