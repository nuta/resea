#include <resea/klog.h>
#include <resea/syscall.h>

error_t klog_write(const char *buf, size_t len) {
    return sys_print(buf, len);
}

int klog_read(char *buf, size_t len) {
    return sys_kdebug("", buf, len);
}
