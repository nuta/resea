//
// Undefined Behavior Sanitizer runtime (UBSan).
// https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
//
#include <types.h>
#include <support/printk.h>
#include <support/ubsan.h>

static void report_ubsan_event(const char *event) {
    PANIC("detected an undefined behavior: %s", event);
}

void __ubsan_handle_type_mismatch_v1(struct ubsan_mismatch_data_v1 *data,
                                     vaddr_t ptr) {
    if (!ptr) {
        report_ubsan_event("NULL pointer dereference");
    } else if (data->align != 0 && (ptr & ((1 << data->align) - 1)) != 0) {
        PANIC("pointer %p is not aligned to %d", ptr, 1 << data->align);
    } else {
        PANIC("pointer %p is not large enough for %s", ptr, data->type->name);
    }
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
