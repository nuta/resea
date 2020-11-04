#ifndef __TEST_H__
#define __TEST_H__
#include <print_macros.h>

extern int failed;

#define TEST_ASSERT(expr)                                                      \
    do {                                                                       \
        if (!(expr)) {                                                         \
            WARN("%s:%d: failed: " #expr, __FILE__, __LINE__);                 \
            failed++;                                                          \
        }                                                                      \
    } while (0)

void ipc_test(void);
void libcommon_test(void);
void libresea_test(void);
void malloc_test(void);
#endif
