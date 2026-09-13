#include "semaphore.h"
#include "../scheduler/scheduler.h"

void sem_init(semaphore_t *s, int value) {
    if (!s) return;
    spinlock_init(&s->lock);
    s->count = value;
    s->wait_head = 0;
    s->wait_tail = 0;
    s->wait_count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        s->wait_queue[i] = NULL;
    }
}

void sem_wait(semaphore_t *s) {
    if (!s) return;

    spinlock_lock(&s->lock);
    s->count--;

    if (s->count < 0) {
        pcb_t *curr = process_get_current();
        if (curr && s->wait_count < MAX_PROCESSES) {
            s->wait_queue[s->wait_head] = curr;
            s->wait_head = (s->wait_head + 1) % MAX_PROCESSES;
            s->wait_count++;
            curr->state = PROCESS_BLOCKED;
        }
        spinlock_unlock(&s->lock);
        scheduler_yield();
    } else {
        spinlock_unlock(&s->lock);
    }
}

void sem_post(semaphore_t *s) {
    if (!s) return;

    spinlock_lock(&s->lock);
    s->count++;

    if (s->count <= 0 && s->wait_count > 0) {
        pcb_t *waiter = s->wait_queue[s->wait_tail];
        s->wait_queue[s->wait_tail] = NULL;
        s->wait_tail = (s->wait_tail + 1) % MAX_PROCESSES;
        s->wait_count--;

        if (waiter) {
            waiter->state = PROCESS_READY;
        }
    }

    spinlock_unlock(&s->lock);
}
