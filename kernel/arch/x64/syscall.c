#include <types.h>
#include <x64/syscall.h>
#include <x64/gdt.h>
#include <x64/msr.h>
#include <x64/asm.h>
#include <x64/handler.h>

STATIC_ASSERT(USER_CS32 + 8 == USER_DS, "SYSRET constraint");
STATIC_ASSERT(USER_CS32 + 16 == USER_CS64, "SYSRET constraint");

void x64_syscall_init(void) {
    asm_wrmsr(MSR_STAR, ((uint64_t) USER_CS32 << 48) | ((uint64_t) KERNEL_CS << 32));
    asm_wrmsr(MSR_LSTAR, (uint64_t) x64_syscall_handler);

    // RIP for compatibility mode. We don't support it for now.
    asm_wrmsr(MSR_CSTAR, 0);
    asm_wrmsr(MSR_SFMASK, 0);

    // Enable SYSCALL/SYSRET.
    asm_wrmsr(MSR_EFER, asm_rdmsr(MSR_EFER) | EFER_SCE);
}
