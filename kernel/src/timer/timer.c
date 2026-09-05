#include "timer.h"
#include "../interrupts/interrupts.h"
#include "../arch/x86_64/io.h"
#include "../console/console.h"
#include "../scheduler/scheduler.h"

static volatile uint64_t system_ticks = 0;
static uint32_t timer_freq = 0;

__attribute__((interrupt)) static void isr_timer(struct interrupt_frame *frame) {
    (void)frame;
    system_ticks++;
    scheduler_tick();
    
    // First 5 ticks always print, then every second
    if (system_ticks <= 5 || system_ticks % timer_freq == 0) {
        console_printf("[TIMER] 1 second passed (Tick: %u)\n", system_ticks);
    }
    
    // Send EOI (End of Interrupt) to Master PIC
    outb(0x20, 0x20);
}

void timer_init(uint32_t frequency) {
    timer_freq = frequency;
    idt_set_descriptor(32, isr_timer, 0x8E);
    
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));

    // Unmask IRQ0 (bit 0) on the Master PIC
    uint8_t current_mask = inb(0x21);
    outb(0x21, current_mask & ~0x01);
}

uint64_t timer_get_ticks(void) {
    return system_ticks;
}
