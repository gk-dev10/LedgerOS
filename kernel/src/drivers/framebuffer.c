#include "framebuffer.h"

static Framebuffer framebuffer;

void framebuffer_init(
    volatile uint32_t *address,
    uint64_t width,
    uint64_t height,
    uint64_t pitch
) {
    framebuffer.address = address;
    framebuffer.width = width;
    framebuffer.height = height;
    framebuffer.pitch = pitch;
}

void framebuffer_clear(uint32_t color) {
    for (size_t y = 0; y < framebuffer.height; y++) {
        for (size_t x = 0; x < framebuffer.width; x++) {
            framebuffer.address[
                y * (framebuffer.pitch / 4) + x
            ] = color;
        }
    }
}

void framebuffer_put_pixel(
    size_t x,
    size_t y,
    uint32_t color
) {
    if (x >= framebuffer.width || y >= framebuffer.height) {
        return;
    }

    framebuffer.address[
        y * (framebuffer.pitch / 4) + x
    ] = color;
}

Framebuffer *framebuffer_get(void) {
    return &framebuffer;
}

uint32_t framebuffer_get_width(void) {
    return (uint32_t)framebuffer.width;
}

uint32_t framebuffer_get_height(void) {
    return (uint32_t)framebuffer.height;
}