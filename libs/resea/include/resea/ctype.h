#ifndef __RESEA_CTYPE_H__
#define __RESEA_CTYPE_H__

#include <types.h>

static inline int toupper(int ch) {
    return ('a' <= ch && ch <= 'z') ? ch - 0x20 : ch;
}

static inline bool isdigit(int ch) {
    return '0' <= ch && ch <= '9';
}

#endif
