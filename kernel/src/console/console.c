#include "console.h"

#include "../framebuffer/framebuffer.h"

#define CONSOLE_FONT_WIDTH   8
#define CONSOLE_FONT_HEIGHT  8

#define DEFAULT_FOREGROUND   0x00FFFFFF
#define DEFAULT_BACKGROUND   0x00101018

/*
 * Temporary built-in minimal font interface.
 *
 * The full embedded font can later replace this.
 */
static const uint8_t basic_font[128][8] = {
    [' '] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    },

    ['.'] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x18, 0x18
    },

    [':'] = {
        0x00, 0x18, 0x18, 0x00,
        0x00, 0x18, 0x18, 0x00
    },

    ['-'] = {
        0x00, 0x00, 0x00, 0x7E,
        0x00, 0x00, 0x00, 0x00
    }
};

static uint64_t cursor_x = 0;
static uint64_t cursor_y = 0;

static uint32_t foreground = DEFAULT_FOREGROUND;
static uint32_t background = DEFAULT_BACKGROUND;

static void console_newline(void) {
    cursor_x = 0;
    cursor_y += CONSOLE_FONT_HEIGHT;

    const framebuffer_t *fb =
        framebuffer_get();

    if (cursor_y + CONSOLE_FONT_HEIGHT >=
        fb->height) {

        /*
         * Temporary behavior.
         *
         * Proper scrolling can be implemented
         * as part of the console module.
         */
        cursor_y = 0;
        framebuffer_clear(background);
    }
}

static void console_draw_char(
    char c,
    uint64_t x,
    uint64_t y
) {
    if (c < 0 || c > 127) {
        c = '?';
    }

    for (uint64_t row = 0;
         row < CONSOLE_FONT_HEIGHT;
         row++) {

        uint8_t line =
            basic_font[(uint8_t)c][row];

        for (uint64_t col = 0;
             col < CONSOLE_FONT_WIDTH;
             col++) {

            uint32_t color =
                (line & (1 << (7 - col)))
                    ? foreground
                    : background;

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

    const framebuffer_t *fb =
        framebuffer_get();

    if (c == '\n') {
        console_newline();
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