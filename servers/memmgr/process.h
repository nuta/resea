#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "elf.h"

#include <types.h>
#include <resea_idl.h>

#define NUM_PROCESSES_MAX 128

struct process {
    pid_t pid;
    const struct initfs_file *file;
    struct elf_file elf;
};

struct initfs_file;
struct process *get_process_by_pid(pid_t pid);
error_t spawn_process(
    cid_t kernel_ch, cid_t server_ch, const struct initfs_file *file);
void process_init(void);

#endif
