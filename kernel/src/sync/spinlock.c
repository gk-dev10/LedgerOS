#include "spinlock.h"

void spinlock_init(spinlock_t *lock) {
    if (lock) {
        lock->locked = 0;
    }
}

void spinlock_lock(spinlock_t *lock) {
    if (!lock) return;
    while (__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE)) {
        asm volatile("pause");
    }
}

void spinlock_unlock(spinlock_t *lock) {
    if (!lock) return;
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);
}
