#include "framebuffer.h"

static framebuffer_t g_framebuffer = {
    .address = NULL,
    .width = 0,
    .height = 0,
    .pitch = 0,
    .initialized = false
};

bool framebuffer_init(
    struct limine_framebuffer *framebuffer
) {
    if (framebuffer == NULL) {
        return false;
    }

    g_framebuffer.address =
        (volatile uint32_t *)framebuffer->address;

    g_framebuffer.width = framebuffer->width;
    g_framebuffer.height = framebuffer->height;

    /*
     * Limine gives pitch in bytes.
     */
    g_framebuffer.pitch =
        framebuffer->pitch / sizeof(uint32_t);

    g_framebuffer.initialized = true;

    return true;
}

void framebuffer_put_pixel(
    uint64_t x,
    uint64_t y,
    uint32_t color
) {
    if (!g_framebuffer.initialized) {
        return;
    }

    if (x >= g_framebuffer.width ||
        y >= g_framebuffer.height) {
        return;
    }

    g_framebuffer.address[
        y * g_framebuffer.pitch + x
    ] = color;
}

void framebuffer_clear(
    uint32_t color
) {
    if (!g_framebuffer.initialized) {
        return;
    }

    for (uint64_t y = 0;
         y < g_framebuffer.height;
         y++) {

        for (uint64_t x = 0;
             x < g_framebuffer.width;
             x++) {

            framebuffer_put_pixel(
                x,
                y,
                color
            );
        }
    }
}

void framebuffer_fill_rect(
    uint64_t x,
    uint64_t y,
    uint64_t width,
    uint64_t height,
    uint32_t color
) {
    if (!g_framebuffer.initialized) {
        return;
    }

    for (uint64_t py = y;
         py < y + height;
         py++) {

        for (uint64_t px = x;
             px < x + width;
             px++) {

            framebuffer_put_pixel(
                px,
                py,
                color
            );
        }
    }
}

const framebuffer_t *framebuffer_get(void) {
    return &g_framebuffer;
}