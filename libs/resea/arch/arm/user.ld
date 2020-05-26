MEMORY {
    /* Copied from libs/resea/arch/arm/user.ld */
    rom : ORIGIN = 0x00011000, LENGTH = 0x30000
    ram : ORIGIN = 0x20002000, LENGTH = 0x2000
}

PHDRS {
    text PT_LOAD;
    data PT_LOAD;
    bss PT_LOAD;
}

SECTIONS {
    /* XXX: For simplicity of the implementation, kernel's ELF loader assumes
            the address and the length of program headers are aligned to 4096
            (PAGE_SIZE). */

    .text :{
        *(.text*)
    } > rom :text

    .rodata :{
        *(.rodata*)
        . = ALIGN(4096); // See the XXX comment above.
    } > rom :text

    .data :{
        *(.data*)
        . = ALIGN(4096); // See the XXX comment above.
    } > ram :data

    .bss :{
        __bss = .;
        *(.bss);
        __bss_end = .;

        . = ALIGN(4);
        __heap = .;
        . += 4096;
        __heap_end = .;

        . = ALIGN(4096); // See the XXX comment above.
    } > ram :bss

    /DISCARD/ :{
        *(.ARM.exidx)
    }
}
