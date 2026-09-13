#ifndef LEDGEROS_GFX_H
#define LEDGEROS_GFX_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void gfx_draw_rect(size_t x, size_t y, size_t width, size_t height, uint32_t color);
void gfx_draw_rect_outline(size_t x, size_t y, size_t width, size_t height, uint32_t color);
void gfx_draw_line(size_t x1, size_t y1, size_t x2, size_t y2, uint32_t color);
void gfx_draw_char(size_t x, size_t y, char c, uint32_t fg, uint32_t bg);
void gfx_draw_string(size_t x, size_t y, const char *str, uint32_t fg, uint32_t bg);
void gfx_draw_gauge(size_t x, size_t y, size_t width, size_t height, uint32_t percentage, uint32_t fg_color, uint32_t bg_color);

#endif
