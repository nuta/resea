#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>

#undef errno
extern int errno;

int printf(const char *fmt, ...);

#define UNIMPLEMENTED()                                                        \
    printf("\x1b[93mnewlib: %s:%d: %s() is not implemented\x1b[0m\n",          \
           __FILE__, __LINE__, __func__);

int gettimeofday(struct timeval *tp, void *tzp) {
    UNIMPLEMENTED();
    return 0;
}

void _exit(int status) {
}

int open(const char *name, int flags, int mode) {
    UNIMPLEMENTED();
    return -1;
}

int close(int file) {
    UNIMPLEMENTED();
    return -1;
}

int read(int file, char *ptr, int len) {
    UNIMPLEMENTED();
    return len;
}

int write(int file, char *ptr, int len) {
    UNIMPLEMENTED();
    return len;
}

int fstat(int file, struct stat *st) {
    UNIMPLEMENTED();
    return 0;
}

int lseek(int file, int ptr, int dir) {
    UNIMPLEMENTED();
    return 0;
}

int getpid(void) {
    UNIMPLEMENTED();
    return 1;
}

int kill(int pid, int sig) {
    UNIMPLEMENTED();
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
    UNIMPLEMENTED();
    return 1;
}

int getentropy(void *ptr, size_t n) {
    return -1;
}

int link(char *old, char *new) {
    errno = EMLINK;
    return -1;
}

void __muloti4(void) {
    UNIMPLEMENTED();
}

void regcomp(void) {
    UNIMPLEMENTED();
}

void regfree(void) {
    UNIMPLEMENTED();
}

void regexec(void) {
    UNIMPLEMENTED();
}

void _jp2uc_l(void) {
    UNIMPLEMENTED();
}

void _uc2jp_l(void) {
    UNIMPLEMENTED();
}

void sigprocmask(void) {
    UNIMPLEMENTED();
}

void unlink(void) {
    UNIMPLEMENTED();
}

void fcntl(void) {
    UNIMPLEMENTED();
}

int mkdir(const char *path, mode_t mode) {
    UNIMPLEMENTED();
    return 0;
}

int stat(const char *__restrict path, struct stat *__restrict sbuf) {
    UNIMPLEMENTED();
    return 0;
}

void times(void) {
    UNIMPLEMENTED();
}

void execve(void) {
    UNIMPLEMENTED();
}

void fork(void) {
    UNIMPLEMENTED();
}

void wait(void) {
    UNIMPLEMENTED();
}

void _init(void) {
    UNIMPLEMENTED();
}

void _fini(void) {
    UNIMPLEMENTED();
}

void sqrtl(void) {
    UNIMPLEMENTED();
}

void __muldc3(void) {
    UNIMPLEMENTED();
}

void __mulsc3(void) {
    UNIMPLEMENTED();
}

void _fe_dfl_env(void) {
    UNIMPLEMENTED();
}

void __fe_dfl_env(void) {
    UNIMPLEMENTED();
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd,
           off_t offset) {
    UNIMPLEMENTED();
    return NULL;
}

int munmap(void *addr, size_t length) {
    UNIMPLEMENTED();
    return 0;
}
