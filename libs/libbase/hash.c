#include <base/hash.h>
#include <base/malloc.h>

static struct hash_element *hash_element_new(opaque_t key, opaque_t value,
                                             struct hash_delete_funcs *funcs) {
    struct hash_element *e = malloc(sizeof(struct hash_element));
    e->key = key;
    e->value = value;
    e->delete_funcs = funcs;
    return e;
}

static void hash_element_delete(struct hash_element *elem) {
    if (elem->delete_funcs->key_delete_func) {
        elem->delete_funcs->key_delete_func(elem->key);
    }

    if (elem->delete_funcs->value_delete_func) {
        elem->delete_funcs->value_delete_func(elem->value);
    }
}

static struct trait hash_element_trait = {
    .delete_func = (delete_func_t) hash_element_delete,
};

static struct hash_element *lookup(hash_t hash, opaque_t key) {
    int bin_index = hash->key_trait->hash_func(key) % hash->num_bins;
    vector_t bin = hash->bins[bin_index];
    vector_foreach_index(bin, i) {
        struct hash_element *e = vector_get(bin, i);
        if (hash->key_trait->equals_func(key, e->key)) {
            return e;
        }
    }

    return NULL;
}

static bool hash_needs_rehash(hash_t hash) {
    return (hash->size / hash->num_bins) >= HASH_REHASH_THRESHOLD;
}

hash_t hash_new(trait_t key_trait, trait_t value_trait) {
    assert(key_trait->hash_func);
    assert(key_trait->equals_func);

    hash_t hash = malloc(sizeof(struct hash));
    hash->bins = malloc(sizeof(vector_t) * HASH_INITIAL_NUM_BINS);
    hash->num_bins = HASH_INITIAL_NUM_BINS;
    hash->size = 0;
    hash->key_trait = key_trait;
    hash->value_trait = value_trait;
    hash->delete_funcs.key_delete_func = key_trait->delete_func;
    hash->delete_funcs.value_delete_func = value_trait->delete_func;
    for (int i = 0; i < HASH_INITIAL_NUM_BINS; i++) {
        hash->bins[i] = vector_new(&hash_element_trait);
    }

    return hash;
}

void hash_delete(hash_t hash) {
    for (int i = 0; i < hash->num_bins; i++) {
        vector_delete(hash->bins[i]);
    }

    free(hash);
}

bool hash_contains(hash_t hash, opaque_t key) {
    struct hash_element *e = lookup(hash, key);
    return e != NULL;
}

opaque_t hash_get(hash_t hash, opaque_t key) {
    struct hash_element *e = lookup(hash, key);
    assert(e != NULL && "non-existent key!");
    return e->value;
}

void hash_set(hash_t hash, opaque_t key, opaque_t value) {
    struct hash_element *new_elem = hash_element_new(key, value,
                                                     &hash->delete_funcs);
    int bin_index = hash->key_trait->hash_func(key) % hash->num_bins;
    vector_t bin = hash->bins[bin_index];

    // Replace an existing element if the key already exists in the hash.
    vector_foreach_index(bin, i) {
        struct hash_element *elem = vector_get(bin, i);
        if (hash->key_trait->equals_func(key, elem->key)) {
            hash_element_delete(elem);
            vector_set(bin, i, new_elem);
            return;
        }
    }

    // Otherwise, append the element into the bin.
    vector_append(bin, new_elem);
    hash->size++;

    if (hash_needs_rehash(hash)) {
        hash_rehash(hash, hash->num_bins * 2);
    }
}

void hash_rehash(hash_t hash, int new_num_bins) {
    vector_t *old_bins = hash->bins;
    int old_num_bins = hash->num_bins;

    vector_t *new_bins = malloc(sizeof(vector_t) * hash->num_bins);
    for (int i = 0; i < new_num_bins; i++) {
        new_bins[i] = vector_new(&hash_element_trait);
    }

    for (int i = 0; i < old_num_bins; i++) {
        vector_t bin = old_bins[i];
        while (!vector_is_empty(bin)) {
            struct hash_element *e = vector_pop(bin);
            hash_set(hash, e->key, e->value);
        }
        vector_delete(bin);
    }

    hash->bins = new_bins;
    hash->num_bins = new_num_bins;
    free(old_bins);
}
