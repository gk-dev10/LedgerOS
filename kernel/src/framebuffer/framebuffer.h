#ifndef LEDGEROS_FRAMEBUFFER_H
#define LEDGEROS_FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

typedef struct {
    volatile uint32_t *address;

    uint64_t width;
    uint64_t height;

    uint64_t pitch;

    bool initialized;
} framebuffer_t;

bool framebuffer_init(
    struct limine_framebuffer *framebuffer
);

void framebuffer_put_pixel(
    uint64_t x,
    uint64_t y,
    uint32_t color
);

void framebuffer_clear(
    uint32_t color
);

void framebuffer_fill_rect(
    uint64_t x,
    uint64_t y,
    uint64_t width,
    uint64_t height,
    uint32_t color
);

const framebuffer_t *framebuffer_get(void);

#endif