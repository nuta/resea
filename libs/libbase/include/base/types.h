#ifndef __BASE_TYPES_H__
#define __BASE_TYPES_H__

#include "sys.h"

typedef void *opaque_t;

typedef void (*delete_func_t)(opaque_t obj);
typedef bool (*equals_func_t)(opaque_t a, opaque_t b);
typedef int (*hash_func_t)(opaque_t key);

struct trait {
    delete_func_t delete_func;
    equals_func_t equals_func;
    hash_func_t hash_func;
};

typedef struct trait *trait_t;

#endif
