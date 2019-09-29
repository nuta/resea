#include <base/malloc.h>

static struct kmalloc_arena object_arena;
static struct kmalloc_arena page_arena;

#define MALLOC_PAGE_ORDER 8

static void append_new_chunk(struct kmalloc_arena *arena, void *buf, size_t size) {
    // Get the end of chunks.
    struct chunk *last_chunk = arena->free_area;
    while (last_chunk && last_chunk->next) {
        last_chunk = last_chunk->next;
    }

    struct chunk *new_chunk = buf;
    new_chunk->next = NULL;
    new_chunk->size = size - sizeof(*new_chunk);
    new_chunk->in_use = 0;
    new_chunk->trailing = 0;

    if (last_chunk) {
        last_chunk->next = new_chunk;
    } else {
        arena->free_area = new_chunk;
    }
}

static void expand_arena(struct kmalloc_arena *arena) {
    void *ptr = sys_alloc_pages(MALLOC_PAGE_ORDER);
    assert(ptr != NULL);
    append_new_chunk(arena, ptr, PAGE_SIZE * POW2(MALLOC_PAGE_ORDER));
}

static void arena_init(struct kmalloc_arena *arena) {
    arena->free_area = NULL;
}

static void split_chunk(struct chunk *chunk, size_t size) {
    size_t remaining_size = chunk->size - size;
    chunk->size = size;

    if (remaining_size <= sizeof(struct chunk)) {
        // The remaining space is too small. Don't create a new
        // chunk header for the remaining part and just track the
        // size of the part.
        chunk->trailing = remaining_size;
    } else {
        // Create a new chunk representing the remaining space.
        struct chunk *remaining = (struct chunk *) &chunk->data[size];
        remaining->next = NULL;
        remaining->in_use = 0;
        remaining->trailing = 0;
        remaining->size = remaining_size - sizeof(struct chunk);
        chunk->trailing = 0;

        remaining->next = chunk->next;
        chunk->next = remaining;
    }
}

static struct chunk *alloc_from_arena(struct kmalloc_arena *arena, size_t size) {
    bool retried = false;

retry:
    // Look for a free space large enough for `size`.
    for (struct chunk *chunk = arena->free_area; chunk; chunk = chunk->next) {
        if (!chunk->in_use && size <= chunk->size) {
            // Found a free space.
            chunk->in_use = 1;
            split_chunk(chunk, size);
            return chunk;
        }
    }

    if (!retried) {
        // Failed to allocate a chunk. Demand another space from the external
        // interface (e.g. from memmgr in Resea).
        expand_arena(&object_arena);
        retried = true;
        goto retry;
    }

    return NULL;
}

static struct chunk *get_chunk_from_pointer(void *ptr) {
    return (struct chunk *) ((uintptr_t) ptr - sizeof(struct chunk));
}

void *malloc(size_t size) {
    // TODO: lock
    if (size == 0) {
        size = 1;
    }

    struct chunk *chunk = alloc_from_arena(&object_arena, size);
    assert(chunk != NULL);
    return chunk->data;
}

void free(void *ptr) {
    // TODO: lock
    struct chunk *chunk = get_chunk_from_pointer(ptr);
    chunk->in_use = 0;

    // TODO: Merge continuous free chunks to mitigrate fragmentation.
}

void base_malloc_init(void) {
    arena_init(&object_arena);
    arena_init(&page_arena);
}
