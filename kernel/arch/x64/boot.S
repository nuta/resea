.section ".boot", "ax"
.intel_syntax noprefix

.code32
.align 8
multiboot2:
    .long 0xe85250d6    /* magic */
    .long 0             /* arch (i386) */
    .long multiboot2_end - multiboot2
    .long   -(0xe85250d6 + 0 + (multiboot2_end - multiboot2))  /* checksum */

    /* terminator */
    .word 0
    .word 0
    .word 0
multiboot2_end:

/* Old fashion multiboot header for QEMU. */
.align 8
old_multiboot:
    .long 0x1badb002        /* magic */
    .long 0                 /* flags */
    .long -(0x1badb002 + 0) /* checksum */
old_multiboot_end:

/* The entry point jumped from the bootloader. */
.code32
.globl x64_boot
x64_boot:
    cli
    cld
    mov esp, 0x7c00

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

    /*
     * Set the boot (later reused for the cpu-local idle thread) stack. It is
     * statically allocated as follows (refer docs/x64/memory-map.md):
     *
     *     address = 0xffff800000a00000 + (cpu_id * PAGE_SIZE)
     *     length  = PAGE_SIZE
     *
     *  Here we assume that the current processor is BSP, its APIC ID is 0,
     *  and PAGE_SIZE is 4096.
     */
    mov rsp, 0xffff800000a00000 + 4096

    /* The kernel no longer access a virtual address around 0x00000000_00000000. */
    mov rdi, 0x0700000
    mov qword ptr [edi], 0 /* unmap */

    /* call x64_setup() */
    mov rax, x64_setup_addr
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
