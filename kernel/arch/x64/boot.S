#include <config.h>
.section ".boot", "ax"
.intel_syntax noprefix

.set MULTIBOOT_MAGIC,    0x1badb002
#ifdef CONFIG_TEXT_UI
.set MULTIBOOT_FLAGS,    0x00000002 // memory map
#else
.set MULTIBOOT_FLAGS,    0x00000006 // memory map and video info
#endif
.set MULTIBOOT_CHECKSUM, -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

.code32

// The multiboot header.
.align 8
.long MULTIBOOT_MAGIC
.long MULTIBOOT_FLAGS
.long MULTIBOOT_CHECKSUM
#ifndef CONFIG_TEXT_UI
.long 0, 0, 0, 0, 0  // Unused values.
.long 0              // Set graphics mode.
.long 640, 480, 32   // Width, height, and depth.
#endif

// The entry point jumped from the bootloader.
.code32
.globl x64_boot
x64_boot:
    cli
    cld

    // Set the boot (later reused for the cpu-local idle thread) stack. It is
    // statically allocated as follows (see INTERNALS.md):
    //
    //     address = 0xffff800000a00000 + (cpu_id * PAGE_SIZE)
    //     length  = PAGE_SIZE
    //
    //  Here we assume that the current processor is BSP, its APIC ID is 0,
    //  and PAGE_SIZE is 4096.
    mov esp, 0xa00000 + 4096

    // Save the 64-bit physical address of struct multiboot_info.
    xor eax, eax
    push eax // Upper 32-bits.
    push ebx

    // Prepare for RETF.
    mov eax, 24
    push eax
    lea edx, [protected_mode]
    push edx

    lgdt [x64_boot_gdtr]
    retf

protected_mode:
    mov ax, 16
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

// Each PDPT table maps 1GiB space. We map 4GiB for kernel to access
// the memory-mapped Local APIC registers.
#define NUM_PDPT_ENTRIES 4

construct_page_table:
    // PML4: 0x00000000_00000000 (temporarily used in protected mode)
    mov edi, 0x0700000
    mov dword ptr [edi], 0x0701003 // Present, writable.
    mov dword ptr [edi + 4], 0

    // PML4: 0xffff8000_00000000
    mov edi, 0x0700000 + 256 * 8
    mov dword ptr [edi], 0x0701003 // Present, writable.
    mov dword ptr [edi + 4], 0

    // PDPT
    mov edi, 0x0701000
    mov eax, 0x0702003 // Present, writable
    mov ecx, NUM_PDPT_ENTRIES

write_pdpt_entry:
    mov dword ptr [edi], eax
    mov dword ptr [edi + 4], 0
    add eax, 0x1000
    add edi, 8
    loop write_pdpt_entry

    // Page Directory
    mov edi, 0x0702000
    mov eax, 0x0083 // Present, writable, page size is 2MB.
    mov ecx, NUM_PDPT_ENTRIES * 512 // (# of PDPT entries) * (# of entries in PD)

write_pd_entry:
    mov dword ptr [edi], eax
    mov dword ptr [edi + 4], 0
    add eax, 0x200000 // 2MB
    add edi, 8
    loop write_pd_entry

    jmp enable_long_mode

#ifdef CONFIG_MP
//
//  AP boot code.
//
.code16
.globl x64_ap_boot, x64_ap_boot_end
x64_ap_boot:
    cli
    mov ax, 0
    mov ds, ax
    mov es, ax

    // Load GDT and enable Protected Mode.
    lgdt [0x5f00]
    mov  eax, cr0
    or   eax, 1
    mov  cr0, eax

    // jmp 24:ap_protected_mode
    .byte 0xea
    .word ap_protected_mode - x64_ap_boot + 0x5000
    .byte 0x18, 0x00

.code32
ap_protected_mode:
    mov ax, 16
    mov ds, ax
    mov es, ax
    mov ss, ax

    //
    // Set the boot (later reused for the cpu-local idle thread) stack:
    //
    //    ESP = 0xa00000 + (cpu_id * PAGE_SIZE).
    //
    mov eax, [0xfee00020] // Get the Local APIC ID.
    shr eax, 24
    shl eax, 12 // EAX * 4096
    add eax, 0xa01000
    mov esp, eax

    lea eax, [enable_long_mode]
    jmp eax
x64_ap_boot_end:
#endif

//
//  Common boot code for both BSP and APs.
//
enable_long_mode:
    // Enable PAE and PGE.
    mov eax, cr4
    or eax, 0xa0
    mov cr4, eax

    // Set the page table address.
    mov eax, 0x0700000
    mov cr3, eax

    // Enable long mode.
    mov ecx, 0xc0000080
    rdmsr
    or eax, 0x0100
    wrmsr

    // Prepare for RETF.
    mov eax, 8
    push eax
    lea edx, [long_mode_in_low_address]
    push edx

    // Enable paging.
    mov eax, cr0
    or  eax, 0x80000000
    mov cr0, eax

    retf

// Temporary GDTR/GDT entries. They are located in the .boot section as theier
// symbol addresses must be physical to load. (in the protected mode, we are not
// yet able to handle addresses above 0xffff800000000000).
.align 8
.globl x64_boot_gdtr
x64_boot_gdtr:
    .word gdt_end - gdt - 1
    .quad gdt

.align 8
gdt:
    .quad 0x0000000000000000 // 0:  null descriptor
    .quad 0x00af9a000000ffff // 8:  64-bit code segment (kernel)
    .quad 0x00cf92000000ffff // 16: 64-bit data segment (kernel)
    .quad 0x00cf9a000000ffff // 24: 32-bit code segment (kernel)
gdt_end:

.code64
long_mode_in_low_address:
    mov ax, 0
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    // Update RSP/RIP to use the virtual address.
    mov rbx, 0xffff800000000000
    or rsp, rbx
    lea rax, [long_mode - 0xffff800000000000]
    or rax, rbx
    jmp rax

//
//  From here, we're in the .text section: we no longer use physical address.
//
.code64
.text
long_mode:
#ifdef CONFIG_MP
    // Determine the current CPU is BSP or AP.
    mov edi, 0xfee00020
    mov eax, [edi]
    shr eax, 24
    test eax, eax
    jz setup_bsp
setup_ap:
    lea rax, [rip + x64_ap_setup]
    call rax
    jmp halt
#endif

setup_bsp:
    // The kernel no longer access a virtual address around 0x0000_0000. Unmap
    // the area to catch bugs (especially NULL pointer dereferences in the
    // kernel).
// FIXME: ap boot code use this in long_mode_in_low_address
//    mov rdi, 0x0700000
//    mov qword ptr [edi], 0 // Clear the PML4 entry to unmap.

    // Clear .bss section
    mov al, 0x00
    lea rdi, [rip + __bss]
    lea rcx, [rip + __bss_end]
    sub rcx, rdi
    cld
    rep stosb

    // Enter x64_bsp_setup
    pop rdi // Restore the address of multiboot_info
    lea rax, [rip + x64_bsp_setup]
    call rax

    // In case x64_setup() returns. I believe it NEVER happen.
halt:
    cli
    hlt
    jmp halt
