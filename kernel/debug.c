#include <arch.h>
#include <debug.h>
#include <ipc.h>
#include <printk.h>
#include <thread.h>

// The symbol table will be embedded by tools/link.py during the build
// process.
extern struct symbol_table __symtable;

/// Resolves the symbol name and the offset from the beginning of symbol.
/// This function always returns "(invalid address)" if the symbol does not
/// exist in the kernel executable.
static const char *find_symbol(vaddr_t vaddr, size_t *offset) {
    ASSERT(
        __symtable.magic == SYMBOL_TABLE_MAGIC && "invalid symbol table magic");

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

/// Prints the stack trace.
void backtrace(void) {
    WARN("Backtrace:");
    struct stack_frame *frame = get_stack_frame();
    for (int i = 0; i < BACKTRACE_MAX; i++) {
        if (frame->return_addr < KERNEL_BASE_ADDR) {
            break;
        }

        size_t offset;
        const char *name = find_symbol(frame->return_addr, &offset);
        WARN("    #%d: %p %s()+0x%x", i, frame->return_addr, name, offset);

        if ((uint64_t) frame->next < KERNEL_BASE_ADDR) {
            break;
        }

        frame = frame->next;
    }
}

/// Returns the pointer to the kernel stack's canary. The stack canary exists
/// at the end (bottom) of the stack. The kernel must not modify it as the
/// kernel stack should be large enough. This function assumes that the size
/// of kernel stack equals to PAGE_SIZE.
static inline vaddr_t get_current_stack_canary_address(void) {
    STATIC_ASSERT(PAGE_SIZE == KERNEL_STACK_SIZE);
    return ALIGN_DOWN(arch_get_stack_pointer(), PAGE_SIZE);
}

/// Writes a kernel stack protection marker. The value `STACK_CANARY` is
/// verified by calling check_stack_canary().
void init_stack_canary(vaddr_t stack_bottom) {
    *((uint32_t *) stack_bottom) = STACK_CANARY;
}

/// Verifies that stack canary is alive. If not so, register a complaint.
void check_stack_canary(void) {
    uint32_t *canary = (uint32_t *) get_current_stack_canary_address();
    if (*canary != STACK_CANARY) {
        PANIC("The stack canary is no more! This is an ex-canary!");
    }
}

/// Each CPU have to call this function once during the boot.
void init_boot_stack_canary(void) {
    init_stack_canary(get_current_stack_canary_address());
}

// Undefined behavior event handlers (UBSan).
// https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
static void report_ubsan_event(const char *event) {
    PANIC("detected an undefined behavior: %s", event);
}

// TODO: Parse type_mismatch_data_v1
void __ubsan_handle_type_mismatch_v1() {
    report_ubsan_event("type_mismatch");
}

void __ubsan_handle_add_overflow() {
    report_ubsan_event("add_overflow");
}
void __ubsan_handle_sub_overflow() {
    report_ubsan_event("sub overflow");
}
void __ubsan_handle_mul_overflow() {
    report_ubsan_event("mul overflow");
}
void __ubsan_handle_divrem_overflow() {
    report_ubsan_event("divrem overflow");
}
void __ubsan_handle_negate_overflow() {
    report_ubsan_event("negate overflow");
}
void __ubsan_handle_pointer_overflow() {
    report_ubsan_event("pointer overflow");
}
void __ubsan_handle_out_of_bounds() {
    report_ubsan_event("out of bounds");
}
void __ubsan_handle_shift_out_of_bounds() {
    report_ubsan_event("shift out of bounds");
}
void __ubsan_handle_builtin_unreachable() {
    report_ubsan_event("builtin unreachable");
}

void debug_init(void) {
}
