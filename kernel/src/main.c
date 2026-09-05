#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "drivers/framebuffer.h"
#include "drivers/serial.h"
#include "console/console.h"
#include "interrupts/interrupts.h"
#include "timer/timer.h"
#include "memory/heap.h"
#include "scheduler/scheduler.h"
#include "ipc/ipc.h"
#include "drivers/keyboard.h"
#include "shell/shell.h"

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
        asm volatile ("hlt");
    }
}

static void print_splash_screen(void) {
    console_set_color(0x0000FFFF, 0x00101018); // Cyan Accent
    console_write("  _           _                 ___  ____  \n");
    console_write(" | |    ___ _| | __ _  ___ _ __/ _ \\/ ___| \n");
    console_write(" | |   / _ `/ _`/ _` |/ _ \\ '__| | | \\___ \\\n");
    console_write(" | |__|  __/ (_| (_| |  __/ |  | |_| |___) |\n");
    console_write(" |_____\\___|\\__,_|\\__, |\\___|_|   \\___/____/ \n");
    console_write("                  |___/                     \n");
    console_set_color(0x00FFFFFF, 0x00101018); // White
    console_write("=====================================================\n");
    console_write("    LedgerOS x86-64 Bootable Operating System v1.0   \n");
    console_write("=====================================================\n\n");
    console_write("  [BOOT] Kernel & Subsystems Initialized Successfully.\n");
    console_write("  [INFO] Type 'help' to view available OS commands.\n\n");
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
    serial_init();
    console_clear();

    interrupts_init();
    heap_init();
    scheduler_init();
    ipc_init();
    timer_init(100); // 100 Hz
    keyboard_init();
    interrupts_enable();

    // Render Splash Screen
    print_splash_screen();

    shell_init();

    // Continuous polling loop
    for (;;) {
        shell_update();
    }
}
