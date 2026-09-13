#include "mutex.h"
#include "../scheduler/scheduler.h"

void mutex_init(mutex_t *m) {
    if (!m) return;
    spinlock_init(&m->lock);
    m->locked = false;
    m->owner = NULL;
    m->wait_head = 0;
    m->wait_tail = 0;
    m->wait_count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        m->wait_queue[i] = NULL;
    }
}

void mutex_lock(mutex_t *m) {
    if (!m) return;

    spinlock_lock(&m->lock);
    pcb_t *curr = process_get_current();

    if (!m->locked) {
        m->locked = true;
        m->owner = curr;
        spinlock_unlock(&m->lock);
        return;
    }

    // Already locked, add current task to wait queue and block
    if (m->wait_count < MAX_PROCESSES && curr) {
        m->wait_queue[m->wait_head] = curr;
        m->wait_head = (m->wait_head + 1) % MAX_PROCESSES;
        m->wait_count++;
        curr->state = PROCESS_BLOCKED;
    }

    spinlock_unlock(&m->lock);
    scheduler_yield();
}

void mutex_unlock(mutex_t *m) {
    if (!m) return;

    spinlock_lock(&m->lock);

    if (m->wait_count > 0) {
        pcb_t *next_owner = m->wait_queue[m->wait_tail];
        m->wait_queue[m->wait_tail] = NULL;
        m->wait_tail = (m->wait_tail + 1) % MAX_PROCESSES;
        m->wait_count--;

        m->owner = next_owner;
        if (next_owner) {
            next_owner->state = PROCESS_READY;
        }
    } else {
        m->locked = false;
        m->owner = NULL;
    }

    spinlock_unlock(&m->lock);
}
