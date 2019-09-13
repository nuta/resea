.section ".boot", "ax"
.intel_syntax noprefix

.set MULTIBOOT_MAGIC,    0x1badb002
.set MULTIBOOT_FLAGS,    0x00000006 // memory map and video info
.set MULTIBOOT_CHECKSUM, -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

.code32

// The multiboot header.
.align 8
.long MULTIBOOT_MAGIC
.long MULTIBOOT_FLAGS
.long MULTIBOOT_CHECKSUM
.long 0, 0, 0, 0, 0  // Unused values.
.long 0              // Set graphics mode.
.long 640, 480, 32   // Width, height, and depth.

/* The entry point jumped from the bootloader. */
.code32
.globl x64_boot
x64_boot:
    cli
    cld

    /*
     * Set the boot (later reused for the cpu-local idle thread) stack. It is
     * statically allocated as follows (see INTERNALS.md):
     *
     *     address = 0xffff800000a00000 + (cpu_id * PAGE_SIZE)
     *     length  = PAGE_SIZE
     *
     *  Here we assume that the current processor is BSP, its APIC ID is 0,
     *  and PAGE_SIZE is 4096.
     */
    mov esp, 0xa00000 + 4096

    /* Save the 64-bit physical address of struct multiboot_info. */
    xor eax, eax
    push eax // Upper 32-bits.
    push ebx

    /* Prepare for RETF. */
    mov eax, 24
    lea edx, [protected_mode]
    push eax
    push edx

    /* Copy GDTR and GDT to a lower address. */
    mov ecx, 0x100
    mov edi, 0x7e00
    lea esi, [gdtr]
    and si, 0xffff
    cld
    rep movsb

    lgdt [0x7e00]
    retf

protected_mode:
    mov ax, 16
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

/*
 * Each PDPT table maps 1GiB space. We map 4GiB for kernel to access
 * the memory-mapped Local APIC registers.
 */
#define NUM_PDPT_ENTRIES 4

construct_page_table:
    /* PML4: 0x00000000_00000000 (temporarily used in protected mode) */
    mov edi, 0x0700000
    mov dword ptr [edi], 0x0701003 /* Present, writable. */
    mov dword ptr [edi + 4], 0

    /* PML4: 0xffff8000_00000000 */
    mov edi, 0x0700000 + 256 * 8
    mov dword ptr [edi], 0x0701003 /* Present, writable. */
    mov dword ptr [edi + 4], 0

    /* PDPT */
    mov edi, 0x0701000
    mov eax, 0x0702003 /* Present, writable */
    mov ecx, NUM_PDPT_ENTRIES

write_pdpt_entry:
    mov dword ptr [edi], eax
    mov dword ptr [edi + 4], 0
    add eax, 0x1000
    add edi, 8
    loop write_pdpt_entry

    /* Page Directory */
    mov edi, 0x0702000
    mov eax, 0x0083 /* Present, writable, page size is 2MB. */
    mov ecx, NUM_PDPT_ENTRIES * 512 /* (# of PDPT entries) * (# of entries in PD) */

write_pd_entry:
    mov dword ptr [edi], eax
    mov dword ptr [edi + 4], 0
    add eax, 0x200000 /* 2MB */
    add edi, 8
    loop write_pd_entry

enable_long_mode:
    /* Enable PAE and PGE. */
    mov eax, cr4
    or eax, 0xa0
    mov cr4, eax

    /* Set the page table address. */
    mov eax, 0x0700000
    mov cr3, eax

    /* Enable long mode. */
    mov ecx, 0xc0000080
    rdmsr
    or eax, 0x0100
    wrmsr

    /* Prepare for RETF. */
    mov eax, 8
    lea edx, [long_mode]
    push eax
    push edx

    /* Enable paging. */
    mov eax, cr0
    or  eax, 0x80000000
    mov cr0, eax

    retf

.code64
long_mode:
    mov ax, 0
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    /* Clear .bss section */
    mov al, 0x00
    mov rdi, bss_addr
    mov rcx, bss_end_addr
    sub rcx, rdi
    cld
    rep stosb

    // Update RSP/RIP to use the virtual address.
    mov rbx, 0xffff800000000000
    or rsp, rbx

    lea rax, [long_mode2]
    or rax, rbx
    jmp rax

long_mode2:
    // Read the address of the x64_setup. Due to a linker error, we cannot use
    // LEA instruction here. Note that x64_setup_addr points to its physical
    // address, not virtual access so we must do this before unmapping the
    // virtual address around 0x0000_0000.
    mov rax, x64_setup_addr

    // The kernel no longer access a virtual address around 0x0000_0000. Unmap
    // the area to catch bugs (especially NULL pointer dereferences in the
    // kernel).
    mov rdi, 0x0700000
    mov qword ptr [edi], 0 /* unmap */

    /* call x64_setup() */
    pop rdi
    call rax

    /* In case main() returns. */
    jmp halt

/* Halt the computer. */
halt:
    cli
    hlt
    jmp halt

.align 8

bss_addr:
    .quad __bss

bss_end_addr:
    .quad __bss_end

x64_setup_addr:
    .quad x64_setup

gdtr:
    .word gdt_end - gdt - 1
    .quad 0x7e0a

gdt:
    .quad 0x0000000000000000 /* 0:  null descriptor */
    .quad 0x00af9a000000ffff /* 8:  64-bit code segment (kernel): 0x0000000000000000 - 0xffffffffffffffff */
    .quad 0x00cf92000000ffff /* 16: 64-bit data segment (kernel): 0x0000000000000000 - 0xffffffffffffffff */
    .quad 0x00cf9a000000ffff /* 24: 32-bit code segment (kernel): 0x00000000         - 0xffffffff */
gdt_end:
