#ifndef LEDGEROS_VMM_H
#define LEDGEROS_VMM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define VMM_FLAG_PRESENT  (1ULL << 0)
#define VMM_FLAG_WRITABLE (1ULL << 1)
#define VMM_FLAG_USER     (1ULL << 2)

void vmm_init(void);
bool vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);

#endif
