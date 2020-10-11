MEMORY {
    /* Don't forget to update libs/resea/arch/arm/user.ld */
    rom : ORIGIN = 0x00000000, LENGTH = 0x10000
    bootelf : ORIGIN = 0x00010000, LENGTH = 0x30000
    ram : ORIGIN = 0x20000000, LENGTH = 0x2000
}

SECTIONS {
    .text :{
        *(.vector*)
        *(.text*)
    } > rom

    .rodata :{
        *(.rodata*)
    } > rom

    .bootelf :{
        *(.bootelf)
    } > bootelf

    .stack :{
        . += 2048;
        __boot_stack_top = .;
    } > ram

    .data :{
        *(.data*)
    } > ram

    .bss :{
        __bss = .;
        *(.bss*)
        __bss_end = .;
    } > ram

    /DISCARD/ :{
        *(.ARM.exidx)
    }
}
