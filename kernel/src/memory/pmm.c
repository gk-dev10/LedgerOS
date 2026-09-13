#include "pmm.h"
#include "../memory.h"
#include "../sync/spinlock.h"

#define PMM_TOTAL_PAGES 2048 // 8 MB managed physical memory arena
static uint8_t pmm_bitmap[PMM_TOTAL_PAGES / 8];
__attribute__((aligned(PAGE_SIZE))) static uint8_t pmm_memory_pool[PMM_TOTAL_PAGES * PAGE_SIZE];

static spinlock_t pmm_lock;

void pmm_init(void) {
    spinlock_init(&pmm_lock);
    memset(pmm_bitmap, 0, sizeof(pmm_bitmap));
}

static inline void bitmap_set(size_t bit) {
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void bitmap_clear(size_t bit) {
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline bool bitmap_test(size_t bit) {
    return (pmm_bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

void *pmm_alloc_page(void) {
    spinlock_lock(&pmm_lock);

    for (size_t i = 0; i < PMM_TOTAL_PAGES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            spinlock_unlock(&pmm_lock);
            void *page_addr = (void *)(pmm_memory_pool + (i * PAGE_SIZE));
            memset(page_addr, 0, PAGE_SIZE);
            return page_addr;
        }
    }

    spinlock_unlock(&pmm_lock);
    return NULL;
}

void pmm_free_page(void *ptr) {
    if (!ptr) return;

    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t pool_start = (uintptr_t)pmm_memory_pool;
    uintptr_t pool_end = pool_start + (PMM_TOTAL_PAGES * PAGE_SIZE);

    if (addr < pool_start || addr >= pool_end) return;

    size_t index = (addr - pool_start) / PAGE_SIZE;

    spinlock_lock(&pmm_lock);
    bitmap_clear(index);
    spinlock_unlock(&pmm_lock);
}

void pmm_get_stats(size_t *total_pages, size_t *used_pages, size_t *free_pages) {
    spinlock_lock(&pmm_lock);
    size_t used = 0;
    for (size_t i = 0; i < PMM_TOTAL_PAGES; i++) {
        if (bitmap_test(i)) used++;
    }
    spinlock_unlock(&pmm_lock);

    if (total_pages) *total_pages = PMM_TOTAL_PAGES;
    if (used_pages) *used_pages = used;
    if (free_pages) *free_pages = PMM_TOTAL_PAGES - used;
}
