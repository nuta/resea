#ifndef __TASK_H__
#define __TASK_H__

#include <types.h>
#include <list.h>

#define SERVICE_NAME_LEN 32

/// Task Control Block (TCB).
struct task {
    bool in_use;
    task_t tid;
    char name[32];
    char cmdline[512];
    struct bootfs_file *file;
    void *file_header;
    struct elf64_ehdr *ehdr;
    struct elf64_phdr *phdrs;
    vaddr_t free_vaddr;
    list_t page_areas;
    vaddr_t ool_buf;
    size_t ool_len;
    task_t received_ool_from;
    vaddr_t received_ool_buf;
    size_t received_ool_len;
    list_t ool_sender_queue;
    list_elem_t ool_sender_next;
    struct message ool_sender_m;
    char waiting_for[SERVICE_NAME_LEN];
};

struct service {
    list_elem_t next;
    char name[SERVICE_NAME_LEN];
    task_t task;
};

task_t task_spawn(struct bootfs_file *file, const char *cmdline);
struct task *task_lookup(task_t tid);
void task_kill(struct task *task);
void service_register(struct task *task, const char *name);
task_t service_wait(struct task *task, const char *name);
void task_init(void);

#endif
