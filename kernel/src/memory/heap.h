#ifndef LEDGEROS_HEAP_H
#define LEDGEROS_HEAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void heap_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void heap_get_stats(size_t *total, size_t *used, size_t *free_bytes);

#endif
