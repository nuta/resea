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

struct framebuffer_info {
    bool available;
    uint64_t paddr;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
};

#define INIT_ARGS_MEMORY_MAPS_MAX 32
struct init_args {
    struct memory_map memory_maps[INIT_ARGS_MEMORY_MAPS_MAX];
    struct framebuffer_info framebuffer;
    int num_memory_maps;
};

#endif
