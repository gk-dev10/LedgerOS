#include "serial.h"
#include "../arch/x86_64/io.h"

#define COM1_PORT 0x3F8

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);    // Disable all interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1_PORT + 1, 0x00);    //                  (hi byte)
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
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
