#include "backtrace.h"

#include <resea.h>

/// The symbol table. This is embedded by tools/link.py during the build.
extern struct symbol_table __symtable;

/// Search the symbol table for the symbol. It never returns NULL; if the given
/// address is out of bounds, it returns the pointer to `"(invalid address)"`.
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

/// These symbols will be added by the linker.
extern char __text[];
extern char __text_end[];

/// Checks whether the address is within the kernel executable region.
static inline bool in_text_section(uintptr_t addr) {
    return ((uintptr_t) __text <= addr && addr < (uintptr_t) __text_end);
}

/// Prints a backtrace.
void do_backtrace(const char *prefix) {
    printf("%sBacktrace:\n", prefix);
    struct stack_frame *frame = get_stack_frame();
    for (int i = 0; frame != NULL && i < 16; i++) {
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
