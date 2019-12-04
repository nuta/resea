#include <arch.h>
#include <channel.h>
#include <support/printk.h>
#include <process.h>
#include <thread.h>
#include <timer.h>
#include <support/kdebug.h>
#include <support/stats.h>

#ifdef DEBUG_BUILD

#define DPRINTK(fmt, ...) printk(COLOR_BOLD fmt COLOR_RESET, ##__VA_ARGS__)

static int strcmp(const char *s1, const char *s2) {
    while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }

    return *s1 - *s2;
}

static void dump_process(struct process *proc) {
    DPRINTK("Process #%d: %s\n", proc->pid, proc->name);
    LIST_FOR_EACH(thread, &proc->threads, struct thread, next) {
        DPRINTK("  Thread %pT: \n", thread);

        struct channel *send_from = thread->debug.send_from;
        if (send_from) {
            DPRINTK("    Sender at %pC <-> %pC => %pC (header=%p)\n",
                    send_from, send_from->linked_with,
                    send_from->linked_with->transfer_to,
                    thread->ipc_buffer->header);
        }

        struct channel *receive_from = thread->debug.receive_from;
        if (receive_from) {
            DPRINTK("    Receiver at %pC <-> %pC => %pC\n",
                    receive_from, receive_from->linked_with,
                    receive_from->linked_with->transfer_to);
        }
    }
}
static void debugger_run(const char *cmdline) {
    if (strcmp(cmdline, "help") == 0) {
        DPRINTK("Kernel debugger commands:\n");
        DPRINTK("\n");
        DPRINTK("  ps - List processes and threads.\n");
        DPRINTK("  st - Show statistics.\n");
        DPRINTK("\n");
    } else if (strcmp(cmdline, "ps") == 0) {
        LIST_FOR_EACH(proc, &process_list, struct process, next) {
            if (proc) {
                dump_process(proc);
            }
        }
    } else if (strcmp(cmdline, "st") == 0) {
        size_t num_used_pages = page_arena.num_objects;
        LIST_FOR_EACH(node, &page_arena.free_list, struct free_list, next) {
            if (node->magic1 != FREE_LIST_MAGIC1
                || node->magic2 != FREE_LIST_MAGIC2) {
                BUG("Corrupted free_list entry %p", node);
            }

            num_used_pages -= node->num_objects;
        }

        size_t num_used_objects = object_arena.num_objects;
        LIST_FOR_EACH(node, &object_arena.free_list, struct free_list, next) {
            if (node->magic1 != FREE_LIST_MAGIC1
                || node->magic2 != FREE_LIST_MAGIC2) {
                BUG("Corrupted free_list entry %p", node);
            }

            num_used_objects -= node->num_objects;
        }

        DPRINTK("\n");

        DPRINTK("  uptime:            %llu\n",
                timer_uptime());
        DPRINTK("  # of IPC:          %llu\n",
                READ_STAT(ipc_total));
        DPRINTK("  # of page faults:  %llu\n",
                READ_STAT(page_fault_total));
        DPRINTK("  # of switches:     %llu\n",
                READ_STAT(context_switch_total));
        DPRINTK("  # of kernel calls: %llu\n",
                READ_STAT(kernel_call_total));

        DPRINTK("\nKernel Memory:\n");
        DPRINTK("  %d of %d (%d%%) pages are in use\n",
                num_used_pages, page_arena.num_objects,
                (num_used_pages * 100) / page_arena.num_objects);
        DPRINTK("  %d of %d (%d%%) objects are in use\n",
                num_used_objects, object_arena.num_objects,
                (num_used_objects * 100) / object_arena.num_objects);

        DPRINTK("\n");
    } else {
        WARN("Invalid debugger command: '%s'.", cmdline);
    }
}

void debugger_oninterrupt(void) {
    static char cmdline[128];
    static unsigned long cursor = 0;
    int ch;
    while ((ch = arch_debugger_readchar()) > 0) {
        if (ch == '\r') {
            printk("\n");
            cmdline[cursor] = '\0';
            if (cursor > 0) {
                debugger_run(cmdline);
                cursor = 0;
            }
            DPRINTK("kdebug> ");
            continue;
        }

        DPRINTK("%c", ch);
        cmdline[cursor++] = (char) ch;

        if (cursor > sizeof(cmdline) - 1) {
            WARN("Too long kernel debugger command.");
            cursor = 0;
        }
    }
}

#endif // DEBUG_BUILD
