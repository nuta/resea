#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <types.h>
#include <string.h>

#define BITS_PER_BYTE   8
#define BITMAP_SIZE(n)  (ALIGN_UP(n, BITS_PER_BYTE) / BITS_PER_BYTE)

void bitmap_fill(uint8_t *bitmap, size_t size, int value);
int bitmap_get(uint8_t *bitmap, size_t size, size_t index);
void bitmap_set(uint8_t *bitmap, size_t size, size_t index);
void bitmap_clear(uint8_t *bitmap, size_t size, size_t index);

#endif
