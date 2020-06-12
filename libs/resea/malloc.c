#include <list.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>

extern char __heap[];
extern char __heap_end[];
static struct malloc_chunk *chunks = NULL;

static void check_buffer_overflow(struct malloc_chunk *chunk) {
    if (chunk->magic == MALLOC_FREE) {
        return;
    }

    for (size_t i = 0; i < MALLOC_REDZONE_LEN; i++) {
        if (chunk->underflow_redzone[i] != MALLOC_REDZONE_UNDFLOW_MARKER) {
            PANIC("detected a malloc buffer underflow: ptr=%p", chunk->data);
        }
    }

    for (size_t i = 0; i < MALLOC_REDZONE_LEN; i++) {
        if (chunk->data[chunk->capacity + i] != MALLOC_REDZONE_OVRFLOW_MARKER) {
            PANIC("detected a malloc buffer overflow: ptr=%p", chunk->data);
        }
    }
}

static struct malloc_chunk *insert(void *ptr, size_t len) {
    ASSERT(len > MALLOC_FRAME_LEN);
    struct malloc_chunk *new_chunk = ptr;
    new_chunk->magic = MALLOC_FREE;
    new_chunk->capacity = len - MALLOC_FRAME_LEN;
    new_chunk->size = 0;
    new_chunk->next = NULL;

    // Append the new chunk into the linked list.
    struct malloc_chunk **chunk = &chunks;
    while (*chunk != NULL) {
        check_buffer_overflow(*chunk);
        chunk = &(*chunk)->next;
    }
    *chunk = new_chunk;

    return new_chunk;
}

static struct malloc_chunk *split(struct malloc_chunk *chunk, size_t len) {
    size_t new_chunk_len = MALLOC_FRAME_LEN + len;
    void *new_chunk =
        &chunk->data[chunk->capacity + MALLOC_REDZONE_LEN - new_chunk_len];
    ASSERT(chunk->capacity >= new_chunk_len);
    chunk->capacity -= new_chunk_len;
    return insert(new_chunk, new_chunk_len);
}

void *malloc(size_t size) {
    if (!size) {
        size = 1;
    }

    // Align up to 16-bytes boundary. Allocate 16 bytes if len equals 0.
    size = ALIGN_UP(size, 16);

    for (struct malloc_chunk *chunk = chunks; chunk; chunk = chunk->next) {
        ASSERT(chunk->magic == MALLOC_FREE || chunk->magic == MALLOC_IN_USE);
        if (chunk->magic != MALLOC_FREE) {
            continue;
        }

        struct malloc_chunk *allocated = NULL;
        if (chunk->capacity > size + MALLOC_FRAME_LEN) {
            allocated = split(chunk, size + MALLOC_FRAME_LEN);
        } else if (chunk->capacity >= size) {
            allocated = chunk;
        }

        if (allocated) {
            allocated->magic = MALLOC_IN_USE;
            allocated->size = size;
            memset(allocated->underflow_redzone, MALLOC_REDZONE_UNDFLOW_MARKER,
                   MALLOC_REDZONE_LEN);
            memset(&allocated->data[allocated->capacity],
                   MALLOC_REDZONE_OVRFLOW_MARKER, MALLOC_REDZONE_LEN);
            return allocated->data;
        }
    }

    PANIC("out of memory");
}

static struct malloc_chunk *get_chunk_from_ptr(void *ptr) {
    struct malloc_chunk *chunk =
        (struct malloc_chunk *) ((uintptr_t) ptr - sizeof(struct malloc_chunk));

    // Check its magic and underflow/overflow redzones.
    ASSERT(chunk->magic == MALLOC_IN_USE);
    check_buffer_overflow(chunk);
    return chunk;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }

    struct malloc_chunk *chunk = get_chunk_from_ptr(ptr);
    if (chunk->capacity <= size) {
        // There's enough room. Keep using the current chunk.
        return ptr;
    }

    // There's not enough room. Allocate a new space and copy old data.
    void *new_ptr = malloc(size);
    memcpy(new_ptr, ptr, chunk->size);
    free(ptr);
    return new_ptr;
}

void free(void *ptr) {
    if (!ptr) {
        return;
    }

    struct malloc_chunk *chunk = get_chunk_from_ptr(ptr);
    if (chunk->magic == MALLOC_FREE) {
        PANIC("double-free bug!");
    }

    chunk->magic = MALLOC_FREE;
}

void malloc_init(void) {
    insert(__heap, (size_t) __heap_end - (size_t) __heap);
}
