#include <list.h>
#include <resea/mymalloc.h>
#include <resea/printf.h>
#include <string.h>

#define NUM_CHUNKS 16
extern char __heap[];
extern char __heap_end[];

static struct malloc_chunk *chunks[NUM_CHUNKS];

static void my_check_buffer_overflow(struct malloc_chunk *chunk) {
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

static struct malloc_chunk *my_insert(void *ptr, size_t len) {
    ASSERT(len > MALLOC_FRAME_LEN);
    struct malloc_chunk *new_chunk = ptr;
    new_chunk->magic = MALLOC_FREE;
    new_chunk->capacity = len - MALLOC_FRAME_LEN;
    new_chunk->size = 0;
    new_chunk->next = NULL;

    // Append the new chunk into the linked list.
    struct malloc_chunk **chunk = &chunks[NUM_CHUNKS - 1];
    while (*chunk != NULL) {
        my_check_buffer_overflow(*chunk);
        chunk = &(*chunk)->next;
    }
    *chunk = new_chunk;

    DBG("Inserted chunk of capacity %d to the last list", new_chunk->capacity);
    return new_chunk;
}

static struct malloc_chunk *my_split(struct malloc_chunk *chunk, size_t len) {
    size_t new_chunk_len = MALLOC_FRAME_LEN + len;
    ASSERT(chunk->capacity >= new_chunk_len);

    void *new_chunk_ptr =
        &chunk->data[chunk->capacity + MALLOC_REDZONE_LEN - new_chunk_len];
    chunk->capacity -= new_chunk_len;

    // DBG("%d", MALLOC_FRAME_LEN);
    ASSERT(new_chunk_len > MALLOC_FRAME_LEN);

    struct malloc_chunk *new_chunk = new_chunk_ptr;
    new_chunk->magic = MALLOC_FREE;
    new_chunk->capacity = len;
    new_chunk->size = 0;
    new_chunk->next = NULL;

    DBG("Going out of split");
    return new_chunk;
    // return insert(new_chunk, new_chunk_len);
}

size_t my_get_chunk_number_from_size(size_t size) {
    // If requested size is less or equal to the size of second largest chunk
    // (the last fixed chunk)
    for (size_t i = 0; i < NUM_CHUNKS; i++) {
        if (size <= 1 << i) {
            return i;
        }
    }
    // if (size <= 1 << (NUM_CHUNKS - 2)) {
    //     return ceil(log2(size));
    // }

    // Return index of the last, dynamic-sized chunk
    return NUM_CHUNKS - 1;
}

void *my_malloc(size_t size) {
    if (!size) {
        size = 1;
    }

    DBG("Inside malloc");

    // Align up to 16-bytes boundary. If the size is less than 16 (including
    // size == 0), allocate 16 bytes.
    size = ALIGN_UP(size, 16);

    size_t chunk_num = my_get_chunk_number_from_size(size);

    if (chunk_num < NUM_CHUNKS - 1) {
        // Check the list corresponding to that size for a free chunk
        // for (struct malloc_chunk *chunk = chunks[chunk_num]; chunk;
        //      chunk = chunk->next) {
        if (chunks[chunk_num] != NULL) {
            struct malloc_chunk *allocated = chunks[chunk_num];
            ASSERT(allocated->magic == MALLOC_FREE);

            // struct malloc_chunk *allocated = chunk;

            allocated->magic = MALLOC_IN_USE;
            allocated->size = size;
            memset(allocated->underflow_redzone, MALLOC_REDZONE_UNDFLOW_MARKER,
                   MALLOC_REDZONE_LEN);
            memset(&allocated->data[allocated->capacity],
                   MALLOC_REDZONE_OVRFLOW_MARKER, MALLOC_REDZONE_LEN);

            chunks[chunk_num] = allocated->next;
            DBG("Allocated chunk of fixed size");
            return allocated->data;
        }

        // if (chunk->capacity > size + MALLOC_FRAME_LEN) {
        //     allocated = split(chunk, size);
        // } else if (chunk->capacity >= size) {
        //     allocated = chunk;
        // }

        // if (allocated) {
        //     allocated->magic = MALLOC_IN_USE;
        //     allocated->size = size;
        //     memset(allocated->underflow_redzone,
        //     MALLOC_REDZONE_UNDFLOW_MARKER,
        //            MALLOC_REDZONE_LEN);
        //     memset(&allocated->data[allocated->capacity],
        //            MALLOC_REDZONE_OVRFLOW_MARKER, MALLOC_REDZONE_LEN);
        //     return allocated->data;
        // }
        // }
    }
    struct malloc_chunk *prev = NULL;
    for (struct malloc_chunk *chunk = chunks[NUM_CHUNKS - 1]; chunk;
         chunk = chunk->next) {
        ASSERT(chunk->magic == MALLOC_FREE);
        // if (chunk->magic != MALLOC_FREE) {
        //     continue;
        // }

        struct malloc_chunk *allocated = NULL;
        if (chunk->capacity > size + MALLOC_FRAME_LEN) {
            DBG("Splitting a large chunk");
            allocated = my_split(chunk, size);
        } else if (chunk->capacity >= size) {
            allocated = chunk;
            DBG("Taking a chunk from last list");
            // Remove chunk from the linked list
            if (prev) {  // If it was not at the head of the list
                prev->next = chunk->next;
            } else {  // If it was at the head of the list
                chunks[NUM_CHUNKS - 1] = chunks[NUM_CHUNKS - 1]->next;
            }
        }

        if (allocated) {
            DBG("Yes allocated");
            allocated->magic = MALLOC_IN_USE;
            allocated->size = size;
            memset(allocated->underflow_redzone, MALLOC_REDZONE_UNDFLOW_MARKER,
                   MALLOC_REDZONE_LEN);
            memset(&allocated->data[allocated->capacity],
                   MALLOC_REDZONE_OVRFLOW_MARKER, MALLOC_REDZONE_LEN);
            DBG("Survived memset");
            allocated->next = NULL;
            return allocated->data;
        }
        prev = chunk;
    }
    PANIC("out of memory");
}

static struct malloc_chunk *my_get_chunk_from_ptr(void *ptr) {
    struct malloc_chunk *chunk =
        (struct malloc_chunk *) ((uintptr_t) ptr - sizeof(struct malloc_chunk));

    // Check its magic and underflow/overflow redzones.
    ASSERT(chunk->magic == MALLOC_IN_USE);
    my_check_buffer_overflow(chunk);
    return chunk;
}

void *my_realloc(void *ptr, size_t size) {
    if (!ptr) {
        return my_malloc(size);
    }

    struct malloc_chunk *chunk = my_get_chunk_from_ptr(ptr);
    if (chunk->capacity <= size) {
        // There's enough room. Keep using the current chunk.
        return ptr;
    }

    // There's not enough room. Allocate a new space and copy old data.
    void *new_ptr = my_malloc(size);
    memcpy(new_ptr, ptr, chunk->size);
    my_free(ptr);
    return new_ptr;
}

void my_free(void *ptr) {
    if (!ptr) {
        return;
    }

    struct malloc_chunk *chunk = my_get_chunk_from_ptr(ptr);
    if (chunk->magic == MALLOC_FREE) {
        PANIC("double-free bug!");
    }

    chunk->magic = MALLOC_FREE;
    size_t chunk_num = my_get_chunk_number_from_size(chunk->capacity);

    struct malloc_chunk *head = chunks[chunk_num];
    chunk->next = head;
    chunks[chunk_num] = chunk;
}

char *my_strndup(const char *s, size_t n) {
    char *new_s = my_malloc(n + 1);
    strncpy(new_s, s, n + 1);
    return new_s;
}

char *my_strdup(const char *s) {
    return my_strndup(s, strlen(s));
}

void my_malloc_init(void) {
    my_insert(__heap, (size_t) __heap_end - (size_t) __heap);
    DBG("%d", (size_t) __heap_end - (size_t) __heap);
}
