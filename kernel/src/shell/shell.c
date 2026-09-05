#include "shell.h"
#include "../console/console.h"
#include "../drivers/keyboard.h"
#include "../timer/timer.h"
#include "../memory.h"
#include "../memory/heap.h"
#include "../scheduler/scheduler.h"
#include "../ipc/ipc.h"
#include "../arch/x86_64/io.h"

#define CMD_BUFFER_SIZE 128

static char cmd_buf[CMD_BUFFER_SIZE];
static uint32_t cmd_len = 0;

static void shell_prompt(void) {
    console_set_color(0x0000FF00, 0x00101018); // Bright Green
    console_write("LedgerOS> ");
    console_set_color(0x00FFFFFF, 0x00101018); // White
}

static void shell_execute(const char *cmd) {
    if (cmd_len == 0) return;

    if (memcmp(cmd, "help", 4) == 0 && (cmd[4] == '\0' || cmd[4] == ' ')) {
        console_write("\n--- LedgerOS Interactive Shell Commands ---\n");
        console_write("  help     - Show this help manual\n");
        console_write("  info     - Display OS kernel version & uptime\n");
        console_write("  clear    - Clear the graphical screen\n");
        console_write("  mem      - Display kernel memory & heap state\n");
        console_write("  ps       - Display active kernel processes & PCBs\n");
        console_write("  sched    - Trigger CPU Scheduler demo\n");
        console_write("  ipc      - Trigger Inter-Process Communication demo\n");
        console_write("  reboot   - Soft reboot the system\n\n");
    } else if (memcmp(cmd, "info", 4) == 0 && (cmd[4] == '\0' || cmd[4] == ' ')) {
        uint64_t ticks = timer_get_ticks();
        console_printf("\n[SYS] LedgerOS v1.0.0 (x86_64 Long Mode)\n");
        console_printf("[SYS] Uptime Ticks: %u (%u seconds)\n", ticks, ticks / 100);
        console_printf("[SYS] Bootloader: Limine Protocol v6\n\n");
    } else if (memcmp(cmd, "clear", 5) == 0 && (cmd[5] == '\0' || cmd[5] == ' ')) {
        console_clear();
        console_write("=========================================\n");
        console_write("              LEDGEROS SHELL\n");
        console_write("=========================================\n\n");
    } else if (memcmp(cmd, "mem", 3) == 0 && (cmd[3] == '\0' || cmd[3] == ' ')) {
        size_t total = 0, used = 0, free_b = 0;
        heap_get_stats(&total, &used, &free_b);
        console_write("\n--- LedgerOS Kernel Heap Memory Stats ---\n");
        console_printf("  Total Heap Size : %u KB (%u bytes)\n", (uint32_t)(total / 1024), (uint32_t)total);
        console_printf("  Used Memory     : %u KB (%u bytes)\n", (uint32_t)(used / 1024), (uint32_t)used);
        console_printf("  Free Memory     : %u KB (%u bytes)\n\n", (uint32_t)(free_b / 1024), (uint32_t)free_b);
    } else if (memcmp(cmd, "ps", 2) == 0 && (cmd[2] == '\0' || cmd[2] == ' ')) {
        pcb_t *plist = NULL;
        size_t total_count = process_get_all(&plist);
        console_write("\nPID  STATE       PRIORITY  TICKS  NAME\n");
        console_write("---  ----------  --------  -----  ------------------------\n");
        for (size_t i = 0; i < total_count; i++) {
            if (plist[i].state != PROCESS_UNUSED) {
                const char *st_str = "UNKNOWN";
                switch (plist[i].state) {
                    case PROCESS_READY:      st_str = "READY"; break;
                    case PROCESS_RUNNING:    st_str = "RUNNING"; break;
                    case PROCESS_BLOCKED:    st_str = "BLOCKED"; break;
                    case PROCESS_TERMINATED: st_str = "TERMINATED"; break;
                    default: break;
                }
                console_printf("%-4u %-11s %-9u %-6u %s\n",
                    plist[i].pid, st_str, plist[i].priority, (uint32_t)plist[i].cpu_ticks, plist[i].name);
            }
        }
        console_write("\n");
    } else if (memcmp(cmd, "sched", 5) == 0 && (cmd[5] == '\0' || cmd[5] == ' ')) {
        scheduler_start_demo();
    } else if (memcmp(cmd, "ipc", 3) == 0 && (cmd[3] == '\0' || cmd[3] == ' ')) {
        ipc_start_demo();
    } else if (memcmp(cmd, "reboot", 6) == 0 && (cmd[6] == '\0' || cmd[6] == ' ')) {
        console_write("\n[SYS] Rebooting system...\n");
        uint8_t good = 0x02;
        while (good & 0x02) {
            good = inb(0x64);
        }
        outb(0x64, 0xFE); // Pulse reset line
    } else {
        console_printf("\nUnknown command: '%s'. Type 'help' for available commands.\n\n", cmd);
    }
}

void shell_init(void) {
    cmd_len = 0;
    memset(cmd_buf, 0, sizeof(cmd_buf));
    shell_prompt();
}

void shell_update(void) {
    while (keyboard_has_char()) {
        char c = keyboard_getchar();

        if (c == '\n') {
            console_putc('\n');
            cmd_buf[cmd_len] = '\0';
            shell_execute(cmd_buf);
            cmd_len = 0;
            memset(cmd_buf, 0, sizeof(cmd_buf));
            shell_prompt();
        } else if (c == '\b') {
            if (cmd_len > 0) {
                cmd_len--;
                cmd_buf[cmd_len] = '\0';
                console_putc('\b');
            }
        } else if (c >= 32 && c <= 126) {
            if (cmd_len < CMD_BUFFER_SIZE - 1) {
                cmd_buf[cmd_len++] = c;
                console_putc(c);
            }
        }
    }
}
