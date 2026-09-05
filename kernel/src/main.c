#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "drivers/framebuffer.h"
#include "console/console.h"
#include "interrupts/interrupts.h"
#include "timer/timer.h"
#include "memory/heap.h"
#include "drivers/keyboard.h"
#include "shell/shell.h"

// Set the base revision to 6, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

static void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    framebuffer_init(
        framebuffer->address,
        framebuffer->width,
        framebuffer->height,
        framebuffer->pitch
    );

    console_init();
    console_clear();
    console_write("=========================================\n");
    console_write("              LEDGEROS\n");
    console_write("=========================================\n\n");
    console_write("[BOOT] Kernel started\n");
    console_write("[ OK ] Framebuffer initialized\n");
    console_write("[ OK ] Console initialized\n");

    interrupts_init();
    console_write("[ OK ] Interrupt subsystem initialized\n");

    heap_init();
    console_write("[ OK ] Kernel heap memory manager initialized (4 MB)\n");

    timer_init(100); // 100 Hz
    console_write("[ OK ] Timer initialized\n");

    keyboard_init();
    console_write("[ OK ] PS/2 Keyboard driver initialized\n\n");

    interrupts_enable();

    shell_init();

    for (;;) {
        shell_update();
        asm ("hlt");
    }
}
