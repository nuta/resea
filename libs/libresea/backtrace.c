#include "backtrace.h"

#include <resea.h>

// The symbol table will be embedded by tools/link.py during the build
// process.
extern struct symbol_table __symtable;

static const char *find_symbol(uintptr_t vaddr, size_t *offset) {
    // Do a binary search. Since `__symtable.num_symbols` is unsigned 16-bit
    // integer, we need larger signed integer to hold -1 here, namely, int32_t.
    int32_t l = -1;
    int32_t r = __symtable.num_symbols;
    while (r - l > 1) {
        // We don't have to care about integer overflow here because
        // `INT32_MIN < -1 <= l + r < UINT16_MAX * 2 < INT32_MAX` always holds.
        //
        // Read this article if you are not familiar with a famous overflow bug:
        // https://ai.googleblog.com/2006/06/extra-extra-read-all-about-it-nearly.html
        int32_t mid = (l + r) / 2;
        if (vaddr >= __symtable.symbols[mid].addr) {
            l = mid;
        } else {
            r = mid;
        }
    }

    if (l == -1) {
        *offset = 0;
        return "(invalid address)";
    } else {
        *offset = vaddr - __symtable.symbols[l].addr;
        return &__symtable.strbuf[__symtable.symbols[l].offset];
    }
}

extern char __text;
extern char __text_end;
static inline bool in_text_section(uintptr_t addr) {
    return ((uintptr_t) &__text <= addr && addr < (uintptr_t) &__text_end);
}

#define BACKTRACE_MAX 16
void do_backtrace(const char *prefix) {
    printf("%sBacktrace:\n", prefix);
    struct stack_frame *frame = get_stack_frame();
    for (int i = 0; frame != NULL && i < BACKTRACE_MAX; i++) {
        if (!in_text_section(frame->return_addr)) {
            break;
        }

        size_t offset;
        const char *name = find_symbol(frame->return_addr, &offset);
        printf("%s    #%d: %p %s()+0x%x\n", prefix, i, frame->return_addr, name,
            offset);

        frame = frame->next;
    }
}
