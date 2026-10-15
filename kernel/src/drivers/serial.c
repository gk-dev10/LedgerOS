#include "serial.h"
#include "../arch/x86_64/io.h"

#define COM1_PORT 0x3F8
#define COM2_PORT 0x2F8

static void serial_configure(uint16_t port) {
    outb(port + 1, 0x00);    // Disable UART interrupts; the kernel polls the port.
    outb(port + 3, 0x80);    // Enable DLAB to set the baud rate divisor.
    outb(port + 0, 0x03);    // Divisor 3: 38400 baud.
    outb(port + 1, 0x00);
    outb(port + 3, 0x03);    // 8 data bits, no parity, one stop bit.
    outb(port + 2, 0xC7);    // Enable and clear FIFO.
    outb(port + 4, 0x0B);    // DTR, RTS, and OUT2.
}

void serial_init(void) {
    serial_configure(COM1_PORT);
}

void serial2_init(void) {
    serial_configure(COM2_PORT);
}

bool serial_has_char(void) {
    return (inb(COM1_PORT + 5) & 0x01) != 0;
}

char serial_getchar(void) {
    if (!serial_has_char()) {
        return 0;
    }
    char c = inb(COM1_PORT);
    if (c == '\r') c = '\n'; // Convert terminal carriage return to newline
    return c;
}

void serial_putc(char c) {
    if (c == '\n') {
        while ((inb(COM1_PORT + 5) & 0x20) == 0) asm volatile("pause");
        outb(COM1_PORT, '\r');
    }
    while ((inb(COM1_PORT + 5) & 0x20) == 0) asm volatile("pause");
    outb(COM1_PORT, c);
}

bool serial2_has_char(void) {
    return (inb(COM2_PORT + 5) & 0x01) != 0;
}

char serial2_getchar(void) {
    if (!serial2_has_char()) {
        return 0;
    }
    char c = inb(COM2_PORT);
    return c == '\r' ? '\n' : c;
}
