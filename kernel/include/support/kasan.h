#ifndef __KASAN_H__
#define __KASAN_H__

#include <types.h>

enum kasan_shadow_tag {
    ASAN_VALID         = 0xc1,
    ASAN_NOT_ALLOCATED = 0xe2,
    ASAN_UNINITIALIZED = 0x77,
    ASAN_FREED         = 0x99,
};

void kasan_init_area(enum kasan_shadow_tag tag, void *ptr, size_t len);
struct free_list;
void kasan_check_double_free(struct free_list *free_list);
void kasan_init(void) ;

#endif
