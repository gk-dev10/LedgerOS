#include "heap.h"
#include "../memory.h"
#include "../console/console.h"

#define HEAP_SIZE (4 * 1024 * 1024) // 4 MB Kernel Heap
#define ALIGNMENT 16

typedef struct heap_block {
    size_t size;
    bool used;
    struct heap_block *next;
} heap_block_t;

__attribute__((aligned(16)))
static uint8_t g_heap_memory[HEAP_SIZE];
static heap_block_t *g_heap_start = NULL;

static size_t align_up(size_t n, size_t align) {
    return (n + align - 1) & ~(align - 1);
}

void heap_init(void) {
    g_heap_start = (heap_block_t *)g_heap_memory;
    g_heap_start->size = HEAP_SIZE - sizeof(heap_block_t);
    g_heap_start->used = false;
    g_heap_start->next = NULL;
}

void *kmalloc(size_t size) {
    if (size == 0 || g_heap_start == NULL) {
        return NULL;
    }

    size_t req_size = align_up(size, ALIGNMENT);
    heap_block_t *curr = g_heap_start;

    while (curr != NULL) {
        if (!curr->used && curr->size >= req_size) {
            // Check if block can be split
            if (curr->size >= req_size + sizeof(heap_block_t) + ALIGNMENT) {
                heap_block_t *new_block = (heap_block_t *)((uint8_t *)curr + sizeof(heap_block_t) + req_size);
                new_block->size = curr->size - req_size - sizeof(heap_block_t);
                new_block->used = false;
                new_block->next = curr->next;

                curr->size = req_size;
                curr->next = new_block;
            }
            curr->used = true;
            return (void *)((uint8_t *)curr + sizeof(heap_block_t));
        }
        curr = curr->next;
    }

    console_write("[ERR] kmalloc Out of Memory!\n");
    return NULL;
}

void kfree(void *ptr) {
    if (ptr == NULL || g_heap_start == NULL) {
        return;
    }

    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    block->used = false;

    // Coalesce / merge adjacent free blocks
    heap_block_t *curr = g_heap_start;
    while (curr != NULL && curr->next != NULL) {
        if (!curr->used && !curr->next->used) {
            curr->size += sizeof(heap_block_t) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

void heap_get_stats(size_t *total, size_t *used, size_t *free_bytes) {
    size_t u = 0;
    size_t f = 0;

    heap_block_t *curr = g_heap_start;
    while (curr != NULL) {
        if (curr->used) {
            u += curr->size + sizeof(heap_block_t);
        } else {
            f += curr->size + sizeof(heap_block_t);
        }
        curr = curr->next;
    }

    if (total) *total = HEAP_SIZE;
    if (used) *used = u;
    if (free_bytes) *free_bytes = f;
}
