#ifndef __BOOTINFO_H__
#define __BOOTINFO_H__

#include <types.h>

enum memory_map_type {
    INITARGS_MEMORY_MAP_FREE,
    INITARGS_MEMORY_MAP_RESERVED,
};

struct memory_map {
    enum memory_map_type type;
    uint64_t start;
    uint64_t end;
};

#define BOOTINFO_MEMORY_MAPS_MAX 32
struct bootinfo {
    struct memory_map memory_maps[BOOTINFO_MEMORY_MAPS_MAX];
    int num_memory_maps;
};

#endif
