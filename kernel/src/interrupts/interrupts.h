#ifndef LEDGEROS_INTERRUPTS_H
#define LEDGEROS_INTERRUPTS_H

#include <stdint.h>

void interrupts_init(void);
void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);

void interrupts_enable(void);

// The interrupt frame pushed by the CPU when an interrupt happens (assuming no privilege change)
struct interrupt_frame {
    uint64_t ip;
    uint64_t cs;
    uint64_t flags;
    uint64_t sp;
    uint64_t ss;
};

#endif
