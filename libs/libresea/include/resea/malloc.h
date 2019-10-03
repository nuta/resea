#ifndef __BASE_MALLOC_H__
#define __BASE_MALLOC_H__

#include <resea.h>

struct chunk {
    /// The pointer to the next chunk.
    struct chunk *next;
    uint32_t size;
    uint16_t in_use:1;
    uint16_t trailing:4;
    uint16_t magic;
    uint8_t data[];
};

STATIC_ASSERT(sizeof(struct chunk) == 16);

struct malloc_arena {
    struct chunk *free_area;
};

void *malloc(size_t size);
void free(void *ptr);
page_base_t valloc(size_t num_pages);

#endif
