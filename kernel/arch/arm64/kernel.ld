ENTRY(boot);

LMA_OFFSET = 0xffff000000000000;
KERNEL_BOOT_ADDR = 0x80000;
KERNEL_BASE_ADDR = 0xffff000000080000;

PHDRS {
	boot PT_LOAD FLAGS(5); /* flags = rx */
	text PT_LOAD FLAGS(5); /* flags = rx */
	data PT_LOAD FLAGS(6); /* flags = rw */
}

SECTIONS {
    . = KERNEL_BOOT_ADDR;
    __kernel_image = .; /* paddr_t */
    .boot : AT(KERNEL_BOOT_ADDR) {
        KEEP(*(.boot));
    } :boot

    . = KERNEL_BASE_ADDR + SIZEOF(.boot);

    .text : AT(ADDR(.text) - LMA_OFFSET) ALIGN(8) {
        *(.text);
        *(.text.*);
    } :text

    .rodata : AT(ADDR(.rodata) - LMA_OFFSET) ALIGN(8) {
        *(.rodata);
        *(.rodata.*);
    } :text

    .data : AT(ADDR(.data) - LMA_OFFSET) ALIGN(8) {
        *(.data);
        *(.data.*);
    } :data

    .bss : AT(ADDR(.bss) - LMA_OFFSET) ALIGN(8) {
        __bss = .;
        *(.bss);
        *(.bss.*);
        __bss_end = .;
    } :data

    . = ALIGN(4096);
    __kernel_image_end = . - KERNEL_BASE_ADDR + KERNEL_BOOT_ADDR; /* paddr_t */

    . = 0x1000000;
    __kernel_data = .; /* paddr_t */

    /* Kernel paging table. */
    . = ALIGN(4096);
    __kernel_page_table = .;
    . += 0x3000;

    /* CPU variables and boot stacks (paddr_t). */
    . = ALIGN(4096);
    __cpuvar_base = .;
    . += 0x4000 * 16; /* CPUVAR_SIZE_MAX * CPU_NUM_MAX */
    __boot_stack_base = .;
    . += 0x1000 * 16; /* PAGE_SIZE * CPU_NUM_MAX */

    /* Kernel Heap (vaddr_t). */
    . = ALIGN(4096);
    . += LMA_OFFSET;
    __kernel_heap = .;
    . = LMA_OFFSET + 0x3000000;
    . = ALIGN(4096);
    __kernel_heap_end = .;

    __kernel_data_end = . - LMA_OFFSET; /* paddr_t */

    /* A temporary page used internally by kernel (virtual address). */
    __temp_page = 0xc000;

    /DISCARD/ :{
        *(*.eh_frame);
    }
}
