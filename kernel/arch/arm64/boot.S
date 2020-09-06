.section ".boot", "ax"
.global boot
boot:
    ldr  x0, =0x30d00800 // Lttile Endian, MMU disabled, I/D-cache disabled
    msr  sctlr_el1, x0
    ldr  x0, =0x80000000 // EL1 is AArch64.
    msr  hcr_el2, x0
    ldr  x0, =0x000001c5 // Switch to EL1h, exceptions are masked.
    msr  spsr_el2, x0

    adr  x0, el1_mode
    msr  elr_el2, x0
    eret

el1_mode:
    // Read the CPU ID.
    mrs  x1, mpidr_el1
    and  x1, x1, #3

    // FIXME:
    ldr  x2, =__boot_stack_base
    ldr  x3, #0x1000
    add  x2, x2, #0x1000
    madd x3, x3, x1, x2 // x3 = x2 + x3 * x1
    mov  sp, x3
    cbz x1, bsp_boot

ap_boot:
    b halt

bsp_boot:
    // Construct the kernel paging table (i.e. TTBR1).
    //
    //   offsets         |    Table
    //  -------------------------------
    //   0x0000-0x1000   |   Level-0
    //   0x1000-0x2000   |   Level-1
    //   0x2000-0x3000   |   Level-2 (0x3000_0000-0x4000_0000)
    //   0x3000-0x4000   |   Level-2 (0x4000_0000-0x5000_0000)
    //
    //   Note: Don't forget to update __kernl_page_table as well to increase the
    //         size of the page table!
    //
    adrp x4, __kernel_page_table

    // Level-0 table
    mov  x0, x4
    add  x1, x4, #0x00001000 // L1 table address.
    ldr  x5, =0x00000403     // Table descriptor.
    orr  x1, x1, x5
    str  x1, [x0]

    // Level-1 table
    add  x0, x4, #0x1000
    add  x1, x4, #0x2000     // L2 table address base. Table descriptor.
    ldr  x5, =0x00000403     // Table descriptor.
    orr  x1, x1, x5
    str  x1, [x0]

    add  x0, x0, #8
    add  x1, x4, #0x3000     // L2 table address base. Table descriptor.
    ldr  x5, =0x00000403     // Table descriptor.
    orr  x1, x1, x5
    str  x1, [x0]

    // Level-2 table: 0x0000_0000-0x3eff_ffff (1008MiB)
    add  x0, x4, #0x2000
    ldr  x1, =0x00000401 // Block descriptor.
    ldr  x2, =504
1:
    str  x1, [x0], 0x8
    add  x1, x1, #0x200000
    sub  x2, x2, 1
    cbnz x2, 1b

    // Level-2 table: 0x3f00_0000-0x4000_0000 (16MiB)
    ldr  x5, =0xffffffe3 // Clear the AttrIndx (the index in MAIR_EL1).
    and  x1, x1, x5
    orr  x1, x1, #0x00000004 // Set AttrIndx to 1.
    ldr  x2, =8
2:
    str  x1, [x0], 0x8
    add  x1, x1, #0x200000
    sub  x2, x2, 1
    cbnz x2, 2b

    // Level-2 table: 0x4000_0000-0x5000_0000 (1GiB)
    add  x0, x4, #0x3000
    ldr  x1, =0x40000401 // Block descriptor.
    ldr  x2, =512
3:
    str  x1, [x0], 0x8
    add  x1, x1, #0x200000
    sub  x2, x2, 1
    cbnz x2, 3b

    // Configure paging.
    ldr  x0, =0x00000044 // 0: Device-nGnRnE memory
                         // 1: Normal memory, FIXME: outer/inner non-cacheble
    msr  mair_el1, x0

    ldr  x0, =0x00100010 // 4KiB pages, the region size is 2^48.
    msr  tcr_el1, x0

    adrp x0, __kernel_page_table
    msr  ttbr1_el1, x0
    msr  ttbr0_el1, x0

enable_mmu:
    adr  x0, mmu_enabled
    ldr  x1, =0xffff000000000000
    orr  x1, x0, x1

    ldr  x0, =0x30d00801 // Lttile Endian, MMU enabled, FIXME: I/D-cache disabled (just for now)
    dsb ish
    isb
    msr  sctlr_el1, x0
    isb
    br   x1

mmu_enabled:
    // Update SP to use the upper address.
    mov  x0, sp
    ldr  x1, =0xffff000000000000
    orr  x0, x0, x1
    mov  sp, x0

    ldr  x0, =arm64_init
    br   x0

halt:
    wfi
    b  halt

.rodata
.balign 4096
.global __bootelf
__bootelf:
    .incbin BOOTELF_PATH
.balign 4096
