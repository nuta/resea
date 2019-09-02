//
//  Undefined behavior event handlers (UBSan).
//  https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
//
#include <resea.h>

static void report_ubsan_event(const char *msg) {
    printf("UBSan: %s\n", msg);
    do_backtrace("");
    exit(1);
}

// TODO: Parse type_mismatch_data_v1
void __ubsan_handle_type_mismatch_v1(UNUSED void *data, uintptr_t ptr) {
    if (ptr)
        report_ubsan_event("type_mismatch");
    else
        report_ubsan_event("NULL pointer dereference");
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
