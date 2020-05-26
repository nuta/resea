#ifndef __RESEA_MAP_H__
#define __RESEA_MAP_H__

#include <types.h>
#include <list.h>

/// An element in a map.
struct map_elem {
    list_t next;
    void *key;
    void *value;
};

/// A string map.
typedef struct {
    /// The number of elements.
    size_t len;
    /// The number of buckets.
    size_t num_buckets;
    /// A list of struct map_elem.
    list_t *buckets;
} *map_t;

#define MAP_REBALANCE_THRESHOLD 8
#define MAP_BUCKET(buckets, num_buckets, key) \
    (&buckets[((size_t) key) % num_buckets])

map_t map_new(void);
void map_delete(map_t map);
size_t map_len(map_t map);
bool map_is_empty(map_t map);
void *map_get(map_t map, void *key);
void *map_set(map_t map, void *key, void *value);
void *map_remove(map_t map, void *key);

#endif
