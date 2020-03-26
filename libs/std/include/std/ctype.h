#ifndef __STD_CTYPE_H__
#define __STD_CTYPE_H__

#include <types.h>

static int toupper(int ch) {
    return ('a' <= ch && ch <= 'z') ? ch - 0x20 : ch;
}

#endif
