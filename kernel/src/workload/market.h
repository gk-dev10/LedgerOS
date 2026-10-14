#ifndef LEDGEROS_MARKET_H
#define LEDGEROS_MARKET_H

#include <stdint.h>
#include <stdbool.h>

void market_init(void);
void market_start_workload(void);
void market_stop_workload(void);

bool market_execute_order(const char *side, const char *ticker, uint32_t qty, uint32_t price, uint64_t *elapsed_cycles);
void market_run_benchmark(uint32_t order_count);

#endif
