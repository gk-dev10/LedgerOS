#ifndef LEDGEROS_KEYBOARD_H
#define LEDGEROS_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

void keyboard_init(void);
bool keyboard_has_char(void);
char keyboard_getchar(void);

#endif
