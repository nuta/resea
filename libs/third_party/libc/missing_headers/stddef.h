#ifndef __STDDEF_H__
#define __STDDEF_H__

#ifndef NULL
#    define NULL ((void *) 0)
#endif

#ifndef offsetof
#    define offsetof(type, member) __builtin_offsetof(type, member)
#endif

#ifndef _SIZE_T_DECLARED
#    define _SIZE_T_DECLARED
typedef unsigned long long size_t;
#endif

typedef long long ptrdiff_t;
typedef int wchar_t;
typedef int wint_t;

#endif
