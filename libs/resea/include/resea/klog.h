#ifndef __RESEA_KLOG_H__
#define __RESEA_KLOG_H__

#include <types.h>

int klog_read(char *buf, size_t len);
error_t klog_write(const char *buf, size_t len);

#endif
