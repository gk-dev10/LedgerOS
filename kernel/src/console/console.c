#include "console.h"
#include "../drivers/framebuffer.h"
#include "../drivers/serial.h"
#include "../memory.h"
#include "font8x8.h"
#include <stdarg.h>
#include <stdbool.h>

#define CONSOLE_FONT_WIDTH   8
#define CONSOLE_FONT_HEIGHT  8

#define DEFAULT_FOREGROUND   0x00FFFFFF
#define DEFAULT_BACKGROUND   0x00101018

static uint64_t cursor_x = 0;
static uint64_t cursor_y = 0;

static uint32_t foreground = DEFAULT_FOREGROUND;
static uint32_t background = DEFAULT_BACKGROUND;

static void console_scroll_up(void) {
    const framebuffer_t *fb = framebuffer_get();
    if (!fb || !fb->address || fb->height <= CONSOLE_FONT_HEIGHT) return;

    size_t pitch_dwords = fb->pitch / 4;
    size_t copy_rows = fb->height - CONSOLE_FONT_HEIGHT;

    // Shift framebuffer pixels up by CONSOLE_FONT_HEIGHT (8 pixels)
    memmove(
        (void *)fb->address,
        (const void *)&fb->address[CONSOLE_FONT_HEIGHT * pitch_dwords],
        copy_rows * fb->pitch
    );

    // Clear the bottom line with background color
    for (size_t y = copy_rows; y < fb->height; y++) {
        for (size_t x = 0; x < fb->width; x++) {
            fb->address[y * pitch_dwords + x] = background;
        }
    }

    cursor_y = copy_rows;
}

static void console_newline(void) {
    cursor_x = 0;
    cursor_y += CONSOLE_FONT_HEIGHT;

    const framebuffer_t *fb = framebuffer_get();

    if (cursor_y + CONSOLE_FONT_HEIGHT >= fb->height) {
        console_scroll_up();
    }
}

static void console_draw_char(
    char c,
    uint64_t x,
    uint64_t y
) {
    uint8_t uc = (uint8_t)c;
    if (uc > 127) {
        uc = '?';
    }

    for (uint64_t row = 0; row < CONSOLE_FONT_HEIGHT; row++) {
        uint8_t line = font8x8_basic[uc][row];

        for (uint64_t col = 0; col < CONSOLE_FONT_WIDTH; col++) {
            uint32_t color = (line & (1 << (7 - col))) ? foreground : background;

            framebuffer_put_pixel(
                x + col,
                y + row,
                color
            );
        }
    }
}

void console_init(void) {

    cursor_x = 0;
    cursor_y = 0;

    foreground = DEFAULT_FOREGROUND;
    background = DEFAULT_BACKGROUND;

    framebuffer_clear(background);
}

void console_clear(void) {

    framebuffer_clear(background);

    cursor_x = 0;
    cursor_y = 0;
}

void console_set_color(
    uint32_t fg,
    uint32_t bg
) {
    foreground = fg;
    background = bg;
}

void console_putc(char c) {
    serial_putc(c);
    const framebuffer_t *fb = framebuffer_get();

    if (c == '\n') {
        console_newline();
        return;
    }

    if (c == '\b') {
        if (cursor_x >= CONSOLE_FONT_WIDTH) {
            cursor_x -= CONSOLE_FONT_WIDTH;
            console_draw_char(' ', cursor_x, cursor_y);
        }
        return;
    }

    console_draw_char(
        c,
        cursor_x,
        cursor_y
    );

    cursor_x += CONSOLE_FONT_WIDTH;

    if (cursor_x + CONSOLE_FONT_WIDTH >=
        fb->width) {

        console_newline();
    }
}

void console_write(const char *text) {

    if (text == NULL) {
        return;
    }

    while (*text) {

        console_putc(*text);

        text++;
    }
}

void console_writeln(const char *text) {
    console_write(text);
    console_putc('\n');
}

static void console_print_number(uint64_t num, int base, bool is_signed) {
    char buf[65];
    char *digits = "0123456789ABCDEF";
    int i = 0;

    if (is_signed && (int64_t)num < 0) {
        console_putc('-');
        num = (uint64_t)(-(int64_t)num);
    }

    if (num == 0) {
        console_putc('0');
        return;
    }

    while (num > 0) {
        buf[i++] = digits[num % base];
        num /= base;
    }

    while (i > 0) {
        console_putc(buf[--i]);
    }
}

void console_printf(const char *fmt, ...) {
    if (!fmt) return;

    va_list args;
    va_start(args, fmt);

    for (size_t i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%') {
            i++;
            if (fmt[i] == '\0') break;

            switch (fmt[i]) {
                case 's': {
                    const char *s = va_arg(args, const char *);
                    console_write(s ? s : "(null)");
                    break;
                }
                case 'd':
                case 'i': {
                    int val = va_arg(args, int);
                    console_print_number((uint64_t)val, 10, true);
                    break;
                }
                case 'u': {
                    unsigned int val = va_arg(args, unsigned int);
                    console_print_number((uint64_t)val, 10, false);
                    break;
                }
                case 'x':
                case 'X': {
                    uint64_t val = va_arg(args, uint64_t);
                    console_print_number(val, 16, false);
                    break;
                }
                case 'p': {
                    uint64_t val = (uint64_t)va_arg(args, void *);
                    console_write("0x");
                    console_print_number(val, 16, false);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    console_putc(c);
                    break;
                }
                case '%': {
                    console_putc('%');
                    break;
                }
                default: {
                    console_putc('%');
                    console_putc(fmt[i]);
                    break;
                }
            }
        } else {
            console_putc(fmt[i]);
        }
    }

    va_end(args);
}

void console_draw_cursor(bool show) {
    const framebuffer_t *fb = framebuffer_get();

    if (!fb || !fb->address) {
        return;
    }

    if (cursor_x + CONSOLE_FONT_WIDTH > fb->width ||
        cursor_y + CONSOLE_FONT_HEIGHT > fb->height) {
        return;
    }

    uint32_t color = show ? foreground : background;

    for (uint64_t row = CONSOLE_FONT_HEIGHT - 2;
         row < CONSOLE_FONT_HEIGHT;
         row++) {

        for (uint64_t col = 0;
             col < CONSOLE_FONT_WIDTH;
             col++) {

            framebuffer_put_pixel(
                cursor_x + col,
                cursor_y + row,
                color
            );
        }
    }
}