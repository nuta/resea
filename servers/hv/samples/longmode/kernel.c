typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned uint32_t;
typedef unsigned long long uint64_t;

#define IOPORT_SERIAL 0x3f8
#define RBR           0
#define DLL           0
#define DLH           1
#define IER           1
#define FCR           2
#define LCR           3
#define LSR           5
#define TX_READY      0x20

static inline void asm_out8(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" ::"a"(value), "Nd"(port));
}

static inline uint8_t asm_in8(uint16_t port) {
    uint8_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void printchar(char ch) {
    while ((asm_in8(IOPORT_SERIAL + LSR) & TX_READY) == 0) {}
    asm_out8(IOPORT_SERIAL, ch);
}

void serial_init(void) {
    int baud = 9600;
    int divisor = 115200 / baud;
    asm_out8(IOPORT_SERIAL + IER, 0x00);  // Disable interrupts.
    asm_out8(IOPORT_SERIAL + DLL, divisor & 0xff);
    asm_out8(IOPORT_SERIAL + DLH, (divisor >> 8) & 0xff);
    asm_out8(IOPORT_SERIAL + LCR, 0x03);  // 8n1.
    asm_out8(IOPORT_SERIAL + FCR, 0x01);  // Enable FIFO.
    asm_out8(IOPORT_SERIAL + IER, 0x01);  // Enable interrupts.
}

void kernel_main(void) {
    serial_init();

    const char *str = "Hello from guest OS!\n";
    while (*str) {
        printchar(*str++);
    }

    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}
