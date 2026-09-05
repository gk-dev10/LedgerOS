#ifndef LEDGEROS_SCHEDULER_H
#define LEDGEROS_SCHEDULER_H

#include "../process/process.h"

void scheduler_init(void);
void scheduler_tick(void);
void scheduler_yield(void);
void scheduler_start_demo(void);

#endif
