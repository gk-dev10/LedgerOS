#ifndef LEDGEROS_MUTEX_H
#define LEDGEROS_MUTEX_H

#include "spinlock.h"
#include "../process/process.h"

typedef struct {
    spinlock_t lock;
    bool locked;
    pcb_t *owner;
    pcb_t *wait_queue[MAX_PROCESSES];
    int wait_head;
    int wait_tail;
    int wait_count;
} mutex_t;

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

#endif
