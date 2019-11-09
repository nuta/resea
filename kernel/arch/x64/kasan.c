#include <arch.h>
#include <x64/apic.h>
#include <support/kasan.h>

#ifdef DEBUG_BUILD

bool arch_asan_is_kernel_address(vaddr_t addr) {
    return addr < 0xffff8000fec00000; // APIC registers.
}

extern char __kernel_image_start[];
extern char __kernel_image_end[];
void arch_asan_init(void) {
    size_t image_len = (size_t) __kernel_image_end
                       - (size_t) __kernel_image_start;

    // Kernel image.
    kasan_init_area(ASAN_VALID, (void *) __kernel_image_start, image_len);
    // AP boot code.
    kasan_init_area(ASAN_VALID, from_paddr(AP_BOOT_CODE_PADDR), 0x1000);
    // QEMU multiboot info.
    kasan_init_area(ASAN_VALID, from_paddr(0x9000), 0x1000);
    // GRUB multiboot info.
    kasan_init_area(ASAN_VALID, from_paddr(0x10000), 0x1000);
    // Text-mode VGA.
    kasan_init_area(ASAN_VALID, from_paddr(0xb8000), 0x1000);
    // MP table.
    kasan_init_area(ASAN_VALID, from_paddr(0xe0000), 0x20000);
    // Kernel's page table (maps above 0xffff8000_00000000).
    kasan_init_area(ASAN_VALID, from_paddr(0x700000), 0x30000);
    // Kernel boot stacks.
    kasan_init_area(ASAN_VALID, (void *) KERNEL_BOOT_STACKS_ADDR,
                  KERNEL_BOOT_STACKS_LEN);
    // CPUVARs.
    kasan_init_area(ASAN_VALID, (void *) CPU_VAR_ADDR, CPU_VAR_LEN);
}

#endif // DEBUG_BUILD
