#ifndef LEDGEROS_FRAMEBUFFER_H
#define LEDGEROS_FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    volatile uint32_t *address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
} Framebuffer;

typedef Framebuffer framebuffer_t;

void framebuffer_init(
    volatile uint32_t *address,
    uint64_t width,
    uint64_t height,
    uint64_t pitch
);

uint32_t framebuffer_get_width(void);
uint32_t framebuffer_get_height(void);

void framebuffer_clear(uint32_t color);

void framebuffer_put_pixel(
    size_t x,
    size_t y,
    uint32_t color
);

Framebuffer *framebuffer_get(void);

#endif