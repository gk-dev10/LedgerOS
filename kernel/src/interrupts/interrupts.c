#include "interrupts.h"
#include "../console/console.h"
#include "../arch/x86_64/io.h"

#define IDT_MAX_DESCRIPTORS 256

typedef struct {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t  ist;
    uint8_t  attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

__attribute__((aligned(0x10))) 
static idt_entry_t idt[IDT_MAX_DESCRIPTORS];

static idtr_t idtr;

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
    uint64_t descriptor = (uint64_t)isr;
    
    uint16_t current_cs = 0;
    asm volatile ("mov %%cs, %0" : "=r"(current_cs));

    idt[vector].isr_low    = (uint16_t)(descriptor & 0xFFFF);
    idt[vector].kernel_cs  = current_cs;
    idt[vector].ist        = 0;
    idt[vector].attributes = flags;
    idt[vector].isr_mid    = (uint16_t)((descriptor >> 16) & 0xFFFF);
    idt[vector].isr_high   = (uint32_t)((descriptor >> 32) & 0xFFFFFFFF);
    idt[vector].reserved   = 0;
}

__attribute__((interrupt)) static void isr_divide_by_zero(struct interrupt_frame *frame) {
    console_printf("\n[EXCEPTION] Divide by zero at 0x%x! System Halted.\n", frame->ip);
    while (1) asm volatile("cli; hlt");
}

__attribute__((interrupt)) static void isr_default_exception(struct interrupt_frame *frame) {
    console_printf("\n[EXCEPTION] Unhandled Exception at 0x%x! System Halted.\n", frame->ip);
    while (1) asm volatile("cli; hlt");
}

__attribute__((interrupt)) static void isr_default(struct interrupt_frame *frame) {
    (void)frame;
}

static void pic_remap(void) {
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);

    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    
    outb(0x21, 0x20); io_wait(); // Master PIC vector offset starts at 32
    outb(0xA1, 0x28); io_wait(); // Slave PIC vector offset starts at 40
    
    outb(0x21, 4); io_wait();    
    outb(0xA1, 2); io_wait();    
    
    outb(0x21, 0x01); io_wait(); 
    outb(0xA1, 0x01); io_wait();

    outb(0x21, a1); 
    outb(0xA1, a2);
}

void interrupts_init(void) {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

    for (uint16_t vector = 0; vector < IDT_MAX_DESCRIPTORS; vector++) {
        if (vector < 32) {
            idt_set_descriptor(vector, isr_default_exception, 0x8E); // 64-bit interrupt gate
        } else {
            idt_set_descriptor(vector, isr_default, 0x8E); 
        }
    }

    idt_set_descriptor(0, isr_divide_by_zero, 0x8E);

    pic_remap();
    
    // Leave ALL hardware IRQs masked initially until their respective drivers are ready.
    outb(0x21, 0xFF); 
    outb(0xA1, 0xFF);

    asm volatile ("lidt %0" : : "m"(idtr));
}

void interrupts_enable(void) {
    asm volatile ("sti");
}
