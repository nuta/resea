OUTPUT_FORMAT("elf64-x86-64");
ENTRY(start);

PHDRS {
    text PT_LOAD;
    data PT_LOAD;
    bss PT_LOAD;
}

SECTIONS {
    . = 0x01000000;

    .text : ALIGN(0x1000) {
        __text = .;
        *(.bootelf_header);
        *(.text);
        *(.text.*);
        __text_end = .;
    } :text

    .rodata : ALIGN(0x1000) {
        *(.rodata);
        *(.rodata.*);
    } :text

    ASSERT ((. < 0x02fff000), "too big .text / .rodata")

    __cmdline = 0x02fff000;

    . = 0x03000000;
    .data : ALIGN(0x1000) {
        *(.data);
        *(.data.*);
    } :data

    ASSERT ((. < 0x04000000), "too big .data")

    . = 0x04000000;
    __zeroed_pages = .;

    .bss : ALIGN(0x1000) {
        __stack = .;
        . += 0x200000;
        __stack_end = .;

        __heap = .;
        . += 0x08000000;
        __heap_end = .;

        __bss = .;
        *(.bss);
        *(.bss.*);
        __bss_end = .;
    } :bss

    ASSERT ((. < 0x10000000), "too big .bss")

    . = 0x10000000;
    __zeroed_pages_end = .;
    __straight_mapping = .;
    __free_vaddr = .;

    . = 0xa0000000000;
    __free_vaddr_end = .;
}
