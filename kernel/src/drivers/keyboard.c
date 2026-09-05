#include "keyboard.h"
#include "serial.h"
#include "../interrupts/interrupts.h"
#include "../arch/x86_64/io.h"

#define KEYBOARD_BUFFER_SIZE 256

static char kbd_buffer[KEYBOARD_BUFFER_SIZE];
static uint32_t kbd_head = 0;
static uint32_t kbd_tail = 0;
static bool shift_pressed = false;

// Standard US QWERTY Scancode map (Set 1)
static const char scancode_ascii_lowercase[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
     0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' '
};

static const char scancode_ascii_uppercase[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
     0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0, '*',   0, ' '
};

static void kbd_push_char(char c) {
    uint32_t next = (kbd_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next != kbd_tail) {
        kbd_buffer[kbd_head] = c;
        kbd_head = next;
    }
}

static void process_scancode(uint8_t scancode) {
    if (scancode == 0x2A || scancode == 0x36) { // Left/Right Shift Pressed
        shift_pressed = true;
    } else if (scancode == 0xAA || scancode == 0xB6) { // Left/Right Shift Released
        shift_pressed = false;
    } else if (!(scancode & 0x80)) { // Key press event
        if (scancode < 128) {
            char c = shift_pressed ? scancode_ascii_uppercase[scancode] : scancode_ascii_lowercase[scancode];
            if (c != 0) {
                kbd_push_char(c);
            }
        }
    }
}

__attribute__((interrupt)) static void isr_keyboard(struct interrupt_frame *frame) {
    (void)frame;

    uint8_t scancode = inb(0x60);
    process_scancode(scancode);

    outb(0x20, 0x20); // Send EOI to PIC master
}

void keyboard_init(void) {
    kbd_head = 0;
    kbd_tail = 0;
    shift_pressed = false;

    // Flush any stale data in PS/2 buffer
    for (int i = 0; i < 16 && (inb(0x64) & 0x01); i++) {
        inb(0x60);
    }

    // Set vector 33 (IRQ1) to keyboard ISR
    idt_set_descriptor(33, isr_keyboard, 0x8E);

    // Unmask IRQ1 (bit 1) on Master PIC
    uint8_t mask = inb(0x21);
    outb(0x21, mask & ~0x02);
}

bool keyboard_has_char(void) {
    if (serial_has_char()) {
        return true;
    }
    if (inb(0x64) & 0x01) {
        uint8_t scancode = inb(0x60);
        process_scancode(scancode);
    }
    return kbd_head != kbd_tail;
}

char keyboard_getchar(void) {
    if (serial_has_char()) {
        return serial_getchar();
    }
    if (kbd_head == kbd_tail) {
        return 0;
    }
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}