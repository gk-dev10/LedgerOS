#ifndef LEDGEROS_CONDVAR_H
#define LEDGEROS_CONDVAR_H

#include "mutex.h"

typedef struct {
    spinlock_t lock;
    pcb_t *wait_queue[MAX_PROCESSES];
    int wait_head;
    int wait_tail;
    int wait_count;
} condvar_t;

void cond_init(condvar_t *cv);
void cond_wait(condvar_t *cv, mutex_t *m);
void cond_signal(condvar_t *cv);
void cond_broadcast(condvar_t *cv);

#endif
