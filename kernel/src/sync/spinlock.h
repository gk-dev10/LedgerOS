#ifndef LEDGEROS_SPINLOCK_H
#define LEDGEROS_SPINLOCK_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    volatile uint32_t locked;
} spinlock_t;

void spinlock_init(spinlock_t *lock);
void spinlock_lock(spinlock_t *lock);
void spinlock_unlock(spinlock_t *lock);

#endif
