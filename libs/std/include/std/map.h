#ifndef __STD_MAP_H__
#define __STD_MAP_H__

#include <types.h>
#include <list.h>
#include <std/string.h>
#include <message.h>

/// An element in a map.
struct map_elem {
    list_t next;
    string_t key;
    void *value;
};

/// A string map. It only accepts string_t as key but I believe it's sufficient
/// for almost usecases.
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
    (&buckets[string_hash(key) % num_buckets])

map_t map_new(void);
void map_delete(map_t map);
size_t map_len(map_t map);
bool map_is_empty(map_t map);
void *map_get(map_t map, string_t key);
void *map_set(map_t map, string_t key, void *value);
void *map_remove(map_t map, string_t key);
void *map_get_handle(map_t map, handle_t *key);
void *map_set_handle(map_t map, handle_t *key, void *value);
void *map_remove_handle(map_t map, handle_t *key);

#endif
