#include <print_macros.h>
#include <bitmap.h>

void bitmap_fill(uint8_t *bitmap, size_t size, int value) {
    memset(bitmap, value ? 0xff : 0x00, size);
}

int bitmap_get(uint8_t *bitmap, size_t size, size_t index) {
    DEBUG_ASSERT(index < size * BITS_PER_BYTE);
    return bitmap[index / BITS_PER_BYTE] >> (index % BITS_PER_BYTE);
}

void bitmap_set(uint8_t *bitmap, size_t size, size_t index) {
    DEBUG_ASSERT(index < size * BITS_PER_BYTE);
    bitmap[index / BITS_PER_BYTE] |=  1 << (index % BITS_PER_BYTE);
}

void bitmap_clear(uint8_t *bitmap, size_t size, size_t index) {
    DEBUG_ASSERT(index < size * BITS_PER_BYTE);
    bitmap[index / BITS_PER_BYTE] &=  ~(1 << (index % BITS_PER_BYTE));
}

