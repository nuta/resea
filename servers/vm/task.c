#include <resea/malloc.h>
#include <resea/task.h>
#include <resea/ipc.h>
#include <string.h>
#include <message.h>
#include <elf.h>
#include "task.h"
#include "bootfs.h"

extern char __free_vaddr[];

static struct task tasks[CONFIG_NUM_TASKS];
static list_t services;

/// Look for the task in the our task table.
struct task *get_task_by_tid(task_t tid) {
    if (tid <= 0 || tid > CONFIG_NUM_TASKS) {
        PANIC("invalid tid %d", tid);
    }

    struct task *task = &tasks[tid - 1];
    ASSERT(task->in_use);
    return task;
}

static void init_task_struct(struct task *task, const char *name,
    struct bootfs_file *file, void *file_header, struct elf64_ehdr *ehdr) {
    task->in_use = true;
    task->file = file;
    task->file_header = file_header;
    task->ehdr = ehdr;
    if (ehdr) {
        task->phdrs = (struct elf64_phdr *) ((uintptr_t) ehdr + ehdr->e_ehsize);
    } else {
        task->phdrs = NULL;
    }

    task->free_vaddr = (vaddr_t) __free_vaddr;
    task->ool_buf = 0;
    task->ool_len = 0;
    task->received_ool_buf = 0;
    task->received_ool_len = 0;
    task->received_ool_from = 0;
    list_init(&task->ool_sender_queue);
    list_nullify(&task->ool_sender_next);
    strncpy(task->name, name, sizeof(task->name));
    strncpy(task->waiting_for, "", sizeof(task->waiting_for));
    list_init(&task->page_areas);
}

task_t launch_task(struct bootfs_file *file) {
    TRACE("launching %s...", file->name);

    // Look for an unused task ID.
    struct task *task = NULL;
    for (int i = 0; i < CONFIG_NUM_TASKS; i++) {
        if (!tasks[i].in_use) {
            task = &tasks[i];
            break;
        }
    }

    if (!task) {
        PANIC("too many tasks");
    }

    void *file_header = malloc(PAGE_SIZE);
    read_file(file, 0, file_header, PAGE_SIZE);

    // Ensure that it's an ELF file.
    struct elf64_ehdr *ehdr = (struct elf64_ehdr *) file_header;
    if (memcmp(ehdr->e_ident, "\x7f" "ELF", 4) != 0) {
        WARN("%s: invalid ELF magic, ignoring...", file->name);
        return ERR_NOT_ACCEPTABLE;
    }

    // Create a new task for the server.
    error_t err =
        task_create(task->tid, file->name, ehdr->e_entry, task_self(), TASK_IO);
    ASSERT_OK(err);

    init_task_struct(task, file->name, file, file_header, ehdr);
    return task->tid;
}

void kill(struct task *task) {
    task_destroy(task->tid);
    task->in_use = false;
    if (task->file_header) {
        free(task->file_header);
    }
}

void service_register(struct task *task, const char *name) {
    // Add the server into the service list.
    struct service *service = malloc(sizeof(*service));
    service->task = task->tid;
    strncpy(service->name, name, sizeof(service->name));
    list_nullify(&service->next);
    list_push_back(&services, &service->next);

    // Look for tasks waiting for the service...
    for (int i = 0; i < CONFIG_NUM_TASKS; i++) {
        struct task *task = &tasks[i];
        if (!strcmp(task->waiting_for, name)) {
            struct message m;
            bzero(&m, sizeof(m));

            m.type = LOOKUP_REPLY_MSG;
            m.lookup_reply.task = service->task;
            ipc_reply(task->tid, &m);

            // The task no longer wait for the service. Clear the field.
            strncpy(task->waiting_for, "", sizeof(task->waiting_for));
        }
    }
}

task_t service_wait(struct task *task, const char *name) {
    LIST_FOR_EACH (s, &services, struct service, next) {
        if (!strcmp(s->name, name)) {
            return s->task;
        }
    }

    // The service is not yet available. Block the caller task until the
    // server is registered by `ipc_serve()`.
    strncpy(task->waiting_for, name, sizeof(task->waiting_for));
    return ERR_WOULD_BLOCK;
}

void task_init(void) {
    for (int i = 0; i < CONFIG_NUM_TASKS; i++) {
        tasks[i].in_use = false;
        tasks[i].tid = i + 1;
    }

    // Initialize a task struct for myself.
    init_task_struct(&tasks[INIT_TASK - 1], "vm", NULL, NULL, NULL);
    list_init(&services);
}
