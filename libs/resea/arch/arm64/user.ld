PHDRS {
    text PT_LOAD;
    data PT_LOAD;
    bss PT_LOAD;
}

SECTIONS {
    /* XXX: For simplicity of the implementation, kernel's ELF loader assumes
            the address and the length of program headers are aligned to 4096
            (PAGE_SIZE). */
    . = 0x100000;

    .text :{
        *(.text*)
    } :text

    .rodata :{
        *(.rodata*)
        . = ALIGN(4096); // See the XXX comment above.
    } :text

    .data :{
        *(.data*)
        . = ALIGN(4096); // See the XXX comment above.
    } :data

    . = 0x300000;
    __zeroed_pages = .;

    .bss :{
        __bss = .;
        *(.bss*);
        __bss_end = .;
        . = ALIGN(4096); // See the XXX comment above.
    } :bss

    . = ALIGN(16);
    __stack = .;
    . += 0x4000;
    __stack_end = .;

    . = ALIGN(16);
    __heap = .;
    . = 0x05000000;
    __heap_end = .;
    __zeroed_pages_end = .;

    __free_vaddr = .;
    . = 0x0a000000;
    __free_vaddr_end = .;

    /DISCARD/ :{
        *(.ARM.exidx)
    }
}
