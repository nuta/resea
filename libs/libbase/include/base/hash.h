#ifndef __BASE_HASH_H__
#define __BASE_HASH_H__

#include <base/types.h>
#include <base/vector.h>

#define HASH_INITIAL_NUM_BINS 8
#define HASH_REHASH_THRESHOLD 4

struct hash_delete_funcs {
    delete_func_t key_delete_func;
    delete_func_t value_delete_func;
};

struct hash_element {
    opaque_t key;
    opaque_t value;
    struct hash_delete_funcs *delete_funcs;
};

struct hash {
    // The array of vector<hash_element>.
    vector_t *bins;
    int num_bins;
    int size;
    trait_t key_trait;
    trait_t value_trait;
    struct hash_delete_funcs delete_funcs;
};

typedef struct hash *hash_t;

hash_t hash_new(trait_t key_trait, trait_t value_trait);
void hash_delete(hash_t hash);
bool hash_contains(hash_t hash, opaque_t key);
opaque_t hash_get(hash_t hash, opaque_t key);
void hash_set(hash_t hash, opaque_t key, opaque_t value);
void hash_rehash(hash_t hash, int new_num_bins);

#endif
