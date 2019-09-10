#ifndef __INIT_ARGS_H__
#define __INIT_ARGS_H__

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

struct init_args {
    struct memory_map memory_maps[16];
    int num_memory_maps;
};

#endif
