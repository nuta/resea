#include <arch.h>
#include <x64/print.h>
#include <x64/x64.h>

void x64_print_init(void) {
    int baud = 9600;
    int divisor = 115200 / baud;
    asm_out8(IOPORT_SERIAL + IER, 0x00); // Disable interrupts
    asm_out8(IOPORT_SERIAL + DLL, divisor & 0xff);
    asm_out8(IOPORT_SERIAL + DLH, (divisor >> 8) & 0xff);
    asm_out8(IOPORT_SERIAL + LCR, 0x03); // 8n1
    asm_out8(IOPORT_SERIAL + FCR, 0x00); // No FIFO
}

static inline void serial_putchar(char ch) {
    while ((asm_in8(IOPORT_SERIAL + LSR) & TX_READY) == 0);
    asm_out8(IOPORT_SERIAL, ch);
}

#define COLOR 0x03
#define TAB_SIZE 4
#define SCREEN_HEIGHT 25
#define SCREEN_WIDTH  80
static inline void screen_putchar(char ch) {
    static int x = 0;
    static int y = 0;
    static bool in_esc = false;
    uint16_t *vram = (uint16_t *) from_paddr(0xb8000);

    // Ignore ANSI escape sequences.
    if (in_esc) {
        in_esc = (ch != 'm');
        return;
    }

    if (ch == '\e') {
        in_esc = true;
        return;
    }

    if (ch == '\n' || x >= SCREEN_WIDTH) {
        x = 0;
        y++;
    }

    if (y >= SCREEN_HEIGHT) {
        // Scroll lines.
        int diff = y - SCREEN_HEIGHT + 1;
        for (int from = diff; from < SCREEN_HEIGHT; from++) {
            inlined_memcpy(vram + (from - diff) * SCREEN_WIDTH,
                           vram + from * SCREEN_WIDTH,
                           SCREEN_WIDTH * sizeof(uint16_t));
        }

        // Clear the new lines.
        inlined_memset(vram + (SCREEN_HEIGHT - diff) * SCREEN_WIDTH, 0,
                       SCREEN_WIDTH * sizeof(uint16_t) * diff);

        y = SCREEN_HEIGHT - 1;
    }

    if (ch == '\t') {
        for (int i = TAB_SIZE - (x % TAB_SIZE); i > 0; i--) {
            screen_putchar(' ');
        }
    } else if (ch != '\n' && ch != '\r') {
        vram[y * SCREEN_WIDTH + x] = (COLOR << 8 | ch);
        x++;
    }

    // Move the cursor.
    int pos = y * SCREEN_WIDTH + x;
	asm_out8(0x3d4, 0x0f);
	asm_out8(0x3d5, pos & 0xff);
	asm_out8(0x3d4, 0x0e);
	asm_out8(0x3d5, (pos >> 8) & 0xff);
}

static inline void do_putchar(char ch) {
    serial_putchar(ch);
    screen_putchar(ch);
}

void arch_putchar(char ch) {
    // Insert '\r' for serial console on QEMU.
    if (ch == '\n') {
        do_putchar('\r');
    }

    do_putchar(ch);
}
