#ifndef LEDGEROS_CONSOLE_H
#define LEDGEROS_CONSOLE_H

#include <stdint.h>

void console_init(void);

void console_clear(void);

void console_putc(char c);

void console_write(const char *text);

void console_writeln(const char *text);

void console_set_color(
    uint32_t foreground,
    uint32_t background
);

#endif