#include <base/malloc.h>
#include <base/vector.h>

vector_t vector_new(trait_t trait) {
    vector_t vec = malloc(sizeof(struct vector));
    vec->buf = malloc(sizeof(opaque_t));
    vec->size = 0;
    vec->capacity = 1;
    vec->trait = trait;
    return vec;
}

void vector_delete(vector_t vec) {
    vector_foreach_index(vec, i) {
        if (vec->trait->delete_func) {
            vec->trait->delete_func(vector_get(vec, i));
        }
    }
    free(vec->buf);
    free(vec);
}

int vector_size(vector_t vec) {
    return vec->size;
}

bool vector_is_empty(vector_t vec) {
    return vec->size == 0;
}

opaque_t vector_get(vector_t vec, int index) {
    return vec->buf[index];
}

void vector_set(vector_t vec, int index, opaque_t data) {
    assert(index < vec->size);
    vec->buf[index] = data;
}

void vector_resize(vector_t vec, int new_capacity) {
    assert(new_capacity > vec->capacity);
    opaque_t *old_buf = vec->buf;
    opaque_t *new_buf = malloc(sizeof(opaque_t) * new_capacity);
    memcpy(new_buf, old_buf, sizeof(opaque_t) * vec->capacity);
    vec->buf = new_buf;
    vec->capacity = new_capacity;
    free(old_buf);
}

void vector_append(vector_t vec, opaque_t data) {
    if (vec->size == vec->capacity) {
        // The vector is full: needs resizing.
        vector_resize(vec, vec->capacity * 2);
    }

    vec->buf[vec->size] = data;
    vec->size++;
}

opaque_t vector_pop(vector_t vec) {
    assert (vec->size > 0);
    opaque_t data = vec->buf[vec->size - 1];
    vec->size--;
    return data;
}
