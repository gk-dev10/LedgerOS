#include "condvar.h"
#include "../scheduler/scheduler.h"

void cond_init(condvar_t *cv) {
    if (!cv) return;
    spinlock_init(&cv->lock);
    cv->wait_head = 0;
    cv->wait_tail = 0;
    cv->wait_count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        cv->wait_queue[i] = NULL;
    }
}

void cond_wait(condvar_t *cv, mutex_t *m) {
    if (!cv || !m) return;

    spinlock_lock(&cv->lock);
    pcb_t *curr = process_get_current();

    if (curr && cv->wait_count < MAX_PROCESSES) {
        cv->wait_queue[cv->wait_head] = curr;
        cv->wait_head = (cv->wait_head + 1) % MAX_PROCESSES;
        cv->wait_count++;
        curr->state = PROCESS_BLOCKED;
    }

    spinlock_unlock(&cv->lock);

    // Release mutex while waiting
    mutex_unlock(m);

    // Yield CPU
    scheduler_yield();

    // Reacquire mutex after being signaled
    mutex_lock(m);
}

void cond_signal(condvar_t *cv) {
    if (!cv) return;

    spinlock_lock(&cv->lock);

    if (cv->wait_count > 0) {
        pcb_t *waiter = cv->wait_queue[cv->wait_tail];
        cv->wait_queue[cv->wait_tail] = NULL;
        cv->wait_tail = (cv->wait_tail + 1) % MAX_PROCESSES;
        cv->wait_count--;

        if (waiter) {
            waiter->state = PROCESS_READY;
        }
    }

    spinlock_unlock(&cv->lock);
}

void cond_broadcast(condvar_t *cv) {
    if (!cv) return;

    spinlock_lock(&cv->lock);

    while (cv->wait_count > 0) {
        pcb_t *waiter = cv->wait_queue[cv->wait_tail];
        cv->wait_queue[cv->wait_tail] = NULL;
        cv->wait_tail = (cv->wait_tail + 1) % MAX_PROCESSES;
        cv->wait_count--;

        if (waiter) {
            waiter->state = PROCESS_READY;
        }
    }

    spinlock_unlock(&cv->lock);
}
