#include "pic.h"
#include "io.h"

#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10

#define ICW4_8086 0x01

void pic_remap(
    uint8_t offset1,
    uint8_t offset2
) {
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    outb(
        PIC1_COMMAND,
        ICW1_INIT | ICW1_ICW4
    );

    io_wait();

    outb(
        PIC2_COMMAND,
        ICW1_INIT | ICW1_ICW4
    );

    io_wait();

    outb(PIC1_DATA, offset1);
    io_wait();

    outb(PIC2_DATA, offset2);
    io_wait();

    outb(PIC1_DATA, 4);
    io_wait();

    outb(PIC2_DATA, 2);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();

    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void pic_send_eoi(
    uint8_t irq
) {
    if (irq >= 8) {
        outb(
            PIC2_COMMAND,
            0x20
        );
    }

    outb(
        PIC1_COMMAND,
        0x20
    );
}

void pic_mask_all(void) {

    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}