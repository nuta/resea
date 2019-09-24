#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "elf.h"
#include <types.h>
#include <resea_idl.h>

struct file;
struct process {
    pid_t pid;
    cid_t pager_ch;
    const struct file *file;
    struct elf_file elf;
};

struct process *get_process_by_pid(pid_t pid);
struct process *get_process_by_cid(cid_t cid);
error_t create_app(cid_t kernel_ch, cid_t memmgr_ch, cid_t appmgr_ch,
                   const struct file *file, pid_t *app_pid);
error_t start_app(cid_t kernel_ch, pid_t app_pid);
void process_init(void);

#endif
