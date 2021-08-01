#include <sys/stat.h>
#include <sys/time.h>

int printf(const char *fmt, ...);

int gettimeofday(struct timeval *tp, void *tzp) {
    printf("\x1b[93mnewlib: %s:%d: %s() is not implemented\x1b[0m\n", __FILE__,
           __LINE__, __func__);
    return 0;
}

void _exit(int status) {
}

int open(const char *name, int flags, int mode) {
    printf("\x1b[93mnewlib: %s:%d: %s() is not implemented\x1b[0m\n", __FILE__,
           __LINE__, __func__);
    return -1;
}

int close(int file) {
    printf("\x1b[93mnewlib: %s:%d: %s() is not implemented\x1b[0m\n", __FILE__,
           __LINE__, __func__);
    return -1;
}

int read(int file, char *ptr, int len) {
    printf("\x1b[93mnewlib: %s:%d: %s() is not implemented\x1b[0m\n", __FILE__,
           __LINE__, __func__);
    return len;
}

int write(int file, char *ptr, int len) {
    printf("\x1b[93mnewlib: %s:%d: %s() is not implemented\x1b[0m\n", __FILE__,
           __LINE__, __func__);
    return len;
}

int fstat(int file, struct stat *st) {
    printf("\x1b[93mnewlib: %s:%d: %s() is not implemented\x1b[0m\n", __FILE__,
           __LINE__, __func__);
    return 0;
}

int lseek(int file, int ptr, int dir) {
    printf("\x1b[93mnewlib: %s:%d: %s() is not implemented\x1b[0m\n", __FILE__,
           __LINE__, __func__);
    return 0;
}

int getpid(void) {
    printf("\x1b[93mnewlib: %s:%d: %s() is not implemented\x1b[0m\n", __FILE__,
           __LINE__, __func__);
    return 1;
}

int kill(int pid, int sig) {
    printf("\x1b[93mnewlib: %s:%d: %s() is not implemented\x1b[0m\n", __FILE__,
           __LINE__, __func__);
    return 0;
}

#define HEAP_SIZE 8 * 1024 * 1024 /* 16 MiB */

/// Allocates a memory chunk for newlib's malloc.
///
/// An application may call newlib's memory allocation functions unimplemented
/// in libs/resea (e.g. calloc).
///
void *sbrk(int incr) {
    static unsigned char heap[HEAP_SIZE];
    static unsigned long current_size = 0;

    if (current_size + incr > HEAP_SIZE) {
        printf("\x1b[93mnewlib: %s:%d: %s(): no memory (incr=%dKiB)\x1b[0m\n",
               __FILE__, __LINE__, __func__, incr / 1024);
        return (void *) -1;
    }

    void *current_break = &heap[current_size];
    current_size += incr;

    return current_break;
}

int isatty(int file) {
    printf("\x1b[93mnewlib: %s:%d: %s() is not implemented\x1b[0m\n", __FILE__,
           __LINE__, __func__);
    return 1;
}
