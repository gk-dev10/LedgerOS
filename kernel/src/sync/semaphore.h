#ifndef LEDGEROS_SEMAPHORE_H
#define LEDGEROS_SEMAPHORE_H

#include "spinlock.h"
#include "../process/process.h"

typedef struct {
    spinlock_t lock;
    int count;
    pcb_t *wait_queue[MAX_PROCESSES];
    int wait_head;
    int wait_tail;
    int wait_count;
} semaphore_t;

void sem_init(semaphore_t *s, int value);
void sem_wait(semaphore_t *s);
void sem_post(semaphore_t *s);

#endif
