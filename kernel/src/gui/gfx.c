#include "gfx.h"
#include "../drivers/framebuffer.h"
#include "../console/font8x8.h"

#define GFX_FONT_SCALE 2

void gfx_draw_rect(size_t x, size_t y, size_t width, size_t height, uint32_t color) {
    uint32_t fb_w = framebuffer_get_width();
    uint32_t fb_h = framebuffer_get_height();

    for (size_t py = y; py < y + height && py < fb_h; py++) {
        for (size_t px = x; px < x + width && px < fb_w; px++) {
            framebuffer_put_pixel(px, py, color);
        }
    }
}

void gfx_draw_rect_outline(size_t x, size_t y, size_t width, size_t height, uint32_t color) {
    if (width == 0 || height == 0) return;
    gfx_draw_rect(x, y, width, 1, color);                  // Top border
    gfx_draw_rect(x, y + height - 1, width, 1, color);     // Bottom border
    gfx_draw_rect(x, y, 1, height, color);                  // Left border
    gfx_draw_rect(x + width - 1, y, 1, height, color);     // Right border
}

/* Bresenham's Integer Line Algorithm (No Floating Point / SSE required) */
void gfx_draw_line(size_t x1, size_t y1, size_t x2, size_t y2, uint32_t color) {
    int x0 = (int)x1;
    int y0 = (int)y1;
    int x1_i = (int)x2;
    int y1_i = (int)y2;

    int dx = (x1_i - x0 > 0) ? (x1_i - x0) : (x0 - x1_i);
    int sx = (x0 < x1_i) ? 1 : -1;
    int dy = (y1_i - y0 > 0) ? -(y1_i - y0) : (y1_i - y0);
    int sy = (y0 < y1_i) ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        framebuffer_put_pixel((size_t)x0, (size_t)y0, color);
        if (x0 == x1_i && y0 == y1_i) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void gfx_draw_char(size_t x, size_t y, char c, uint32_t fg, uint32_t bg) {
    uint8_t u_c = (uint8_t)c;
    if (u_c >= 128) u_c = '?';

    const uint8_t *glyph = font8x8_basic[u_c];

    for (size_t row = 0; row < 8; row++) {
        uint8_t glyph_row = glyph[row];
        for (size_t col = 0; col < 8; col++) {
            // Bit 7 is leftmost pixel, Bit 0 is rightmost pixel
            bool pixel_set = (glyph_row & (1 << (7 - col))) != 0;
            uint32_t color = pixel_set ? fg : bg;

            if (pixel_set || bg != 0xFFFFFFFF) {
                for (size_t dy = 0; dy < GFX_FONT_SCALE; dy++) {
                    for (size_t dx = 0; dx < GFX_FONT_SCALE; dx++) {
                        framebuffer_put_pixel(
                            x + (col * GFX_FONT_SCALE) + dx,
                            y + (row * GFX_FONT_SCALE) + dy,
                            color
                        );
                    }
                }
            }
        }
    }
}

void gfx_draw_string(size_t x, size_t y, const char *str, uint32_t fg, uint32_t bg) {
    if (!str) return;
    size_t curr_x = x;
    while (*str) {
        if (*str == '\n') {
            curr_x = x;
            y += (8 * GFX_FONT_SCALE + 2);
        } else {
            gfx_draw_char(curr_x, y, *str, fg, bg);
            curr_x += (8 * GFX_FONT_SCALE);
        }
        str++;
    }
}

void gfx_draw_gauge(size_t x, size_t y, size_t width, size_t height, uint32_t percentage, uint32_t fg_color, uint32_t bg_color) {
    if (percentage > 100) percentage = 100;
    gfx_draw_rect(x, y, width, height, bg_color);
    size_t filled_w = (width * percentage) / 100;
    if (filled_w > 0) {
        gfx_draw_rect(x, y, filled_w, height, fg_color);
    }
    gfx_draw_rect_outline(x, y, width, height, 0x00505060);
}
