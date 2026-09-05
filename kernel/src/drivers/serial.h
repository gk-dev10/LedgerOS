#ifndef LEDGEROS_SERIAL_H
#define LEDGEROS_SERIAL_H

#include <stdint.h>
#include <stdbool.h>

void serial_init(void);
bool serial_has_char(void);
char serial_getchar(void);
void serial_putc(char c);

#endif
