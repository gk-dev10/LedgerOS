#include "shell.h"
#include "../console/console.h"
#include "../drivers/keyboard.h"
#include "../drivers/serial.h"
#include "../timer/timer.h"
#include "../memory.h"
#include "../memory/heap.h"
#include "../memory/pmm.h"
#include "../memory/vmm.h"
#include "../scheduler/scheduler.h"
#include "../ipc/ipc.h"
#include "../workload/market.h"
#include "../fs/ledgerfs.h"
#include "../gui/gui.h"
#include "../arch/x86_64/io.h"
#include "../sync/spinlock.h"

#define CMD_BUFFER_SIZE 128

static char cmd_buf[CMD_BUFFER_SIZE];
static uint32_t cmd_len = 0;

static void shell_print_field(const char *text, size_t width) {
    size_t len = 0;
    while (text[len]) {
        console_putc(text[len++]);
    }
    while (len++ < width) {
        console_putc(' ');
    }
}

static void shell_print_uint_field(uint32_t value, size_t width) {
    char digits[11];
    size_t len = 0;
    do {
        digits[len++] = '0' + (value % 10);
        value /= 10;
    } while (value && len < sizeof(digits));
    for (size_t i = len; i > 0; i--) {
        console_putc(digits[i - 1]);
    }
    while (len++ < width) {
        console_putc(' ');
    }
}

static void shell_prompt(void) {
    console_set_color(0x0000FF00, 0x00101018); // Bright Green
    console_write("LedgerOS> ");
    console_set_color(0x00FFFFFF, 0x00101018); // White
}

static int shell_atoi(const char *str) {
    int res = 0;
    while (*str == ' ') str++;
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}

static uintptr_t shell_atou_hex(const char *str) {
    uintptr_t res = 0;
    while (*str == ' ') str++;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) str += 2;
    while (*str) {
        char c = *str;
        if (c >= '0' && c <= '9') {
            res = (res << 4) | (uintptr_t)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            res = (res << 4) | (uintptr_t)(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            res = (res << 4) | (uintptr_t)(c - 'A' + 10);
        } else {
            break;
        }
        str++;
    }
    return res;
}

static void dummy_process_entry(void) {
    for (;;) {
        asm volatile ("hlt");
    }
}

// Race condition demo state. Workers explicitly yield in the vulnerable window
// so the lost update is repeatable and visible in the classroom demonstration.
#define RACE_DEMO_ITERATIONS 3000
#define RACE_DEMO_REPORT_EVERY 500
static volatile int race_counter = 0;
static volatile uint32_t race_locked_counter = 0;
static volatile bool race_demo_use_lock = false;
static spinlock_t race_demo_lock;

static void race_demo_worker(uint32_t worker_id) {
    for (uint32_t i = 1; i <= RACE_DEMO_ITERATIONS; i++) {
        if (race_demo_use_lock) {
            spinlock_lock(&race_demo_lock);
            race_locked_counter++;
            spinlock_unlock(&race_demo_lock);
            // Let the other worker run after the protected increment.
            scheduler_yield();
        } else {
            int snapshot = race_counter;
            // Force an interleaving between the read and write of the counter.
            scheduler_yield();
            race_counter = snapshot + 1;
        }

        if (i % RACE_DEMO_REPORT_EVERY == 0) {
            uint32_t value = race_demo_use_lock ? race_locked_counter : (uint32_t)race_counter;
            console_printf("[RACE] Worker %u progress %u/%u | shared counter %u\n",
                worker_id, i, RACE_DEMO_ITERATIONS, value);
        }
    }
    console_printf("[RACE] Worker %u complete.\n", worker_id);
}

static void race_demo_worker_one(void) { race_demo_worker(1); }
static void race_demo_worker_two(void) { race_demo_worker(2); }

static bool race_demo_run_workers(void) {
    pcb_t *worker_one = process_create("RaceWorker-1", race_demo_worker_one, 6);
    pcb_t *worker_two = process_create("RaceWorker-2", race_demo_worker_two, 6);
    if (!worker_one || !worker_two) {
        if (worker_one) {
            kfree(worker_one->stack);
            worker_one->stack = NULL;
            worker_one->state = PROCESS_UNUSED;
        }
        if (worker_two) {
            kfree(worker_two->stack);
            worker_two->stack = NULL;
            worker_two->state = PROCESS_UNUSED;
        }
        console_write("[ERR] Could not create both race demo workers; check process table capacity.\n");
        return false;
    }

    // The scheduler returns here after both workers finish and yield back.
    scheduler_yield();
    return true;
}

void shell_execute(const char *cmd) {
    if (!cmd || cmd[0] == '\0') return;

    if (memcmp(cmd, "help", 4) == 0 && (cmd[4] == '\0' || cmd[4] == ' ')) {
        console_write("\n--- LedgerOS Interactive Shell Commands ---\n");
        console_write("  help                 - Show this help manual\n");
        console_write("  info                 - Display OS kernel version & uptime\n");
        console_write("  clear                - Clear the graphical screen\n");
        console_write("  gui                  - Launch live real-time OS graphical dashboard\n");
        console_write("  mem                  - Display kernel memory & heap state\n");
        console_write("  mem chunks           - Inspect heap chunks & detect memory fragmentation\n");
        console_write("  vmm stats            - Display physical & virtual page memory stats\n");
        console_write("  ps                   - Display active kernel processes & PCBs\n");
        console_write("  sched                - Trigger CPU Scheduler demo\n");
        console_write("  ipc                  - Trigger Inter-Process Communication demo\n");
        console_write("  market start         - Start autonomous 4-process IPC market workload\n");
        console_write("  market stop          - Stop market workload pipeline\n");
        console_write("  log view             - Display audit records from LedgerFS ramdisk\n");
        console_write("  cat <path>           - Inspect log file contents (e.g. cat /var/log/market.log)\n");
        console_write("  spawn <name>         - Dynamically create a new PCB process\n");
        console_write("  renice <pid> <prio>  - Alter process scheduling priority (0-10) live\n");
        console_write("  alloc <bytes>        - Dynamically allocate memory from 4MB heap\n");
        console_write("  free <hex_addr>      - Free allocated heap memory chunk & coalesce\n");
        console_write("  race demo            - Fault injection: demonstrate unsynchronized race condition\n");
        console_write("  order <buy|sell> <t> <q> <p> - Place mock order with hardware rdtsc latency\n");
        console_write("  bench [count]        - Run order latency benchmark & Windows comparison\n");
        console_write("  msg <ticker> <price> - Send custom live market message to IPC mailbox\n");
        console_write("  reboot               - Soft reboot the system\n\n");
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
    } else if (memcmp(cmd, "gui", 3) == 0 && (cmd[3] == '\0' || cmd[3] == ' ')) {
        gui_run_live_mode();
    } else if (memcmp(cmd, "mem chunks", 10) == 0 && (cmd[10] == '\0' || cmd[10] == ' ')) {
        heap_dump_blocks();
    } else if (memcmp(cmd, "mem", 3) == 0 && (cmd[3] == '\0' || cmd[3] == ' ')) {
        size_t total = 0, used = 0, free_b = 0;
        heap_get_stats(&total, &used, &free_b);
        console_write("\n--- LedgerOS Kernel Heap Memory Stats ---\n");
        console_printf("  Total Heap Size : %u KB (%u bytes)\n", (uint32_t)(total / 1024), (uint32_t)total);
        console_printf("  Used Memory     : %u KB (%u bytes)\n", (uint32_t)(used / 1024), (uint32_t)used);
        console_printf("  Free Memory     : %u KB (%u bytes)\n\n", (uint32_t)(free_b / 1024), (uint32_t)free_b);
    } else if (memcmp(cmd, "vmm stats", 9) == 0 && (cmd[9] == '\0' || cmd[9] == ' ')) {
        size_t total_p = 0, used_p = 0, free_p = 0;
        pmm_get_stats(&total_p, &used_p, &free_p);
        console_write("\n--- LedgerOS Physical Frame Allocator (PMM) Stats ---\n");
        console_printf("  Total 4KB Frames : %u (%u KB)\n", (uint32_t)total_p, (uint32_t)(total_p * 4));
        console_printf("  Used Frames      : %u (%u KB)\n", (uint32_t)used_p, (uint32_t)(used_p * 4));
        console_printf("  Free Frames      : %u (%u KB)\n\n", (uint32_t)free_p, (uint32_t)(free_p * 4));
    } else if (memcmp(cmd, "ps", 2) == 0 && (cmd[2] == '\0' || cmd[2] == ' ')) {
        pcb_t *plist = NULL;
        size_t total_count = process_get_all(&plist);
        console_write("\nPID   STATE        PRIORITY  TICKS   NAME\n");
        console_write("----- ------------ --------- ------- ------------------------\n");
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
                shell_print_uint_field(plist[i].pid, 6);
                shell_print_field(st_str, 13);
                shell_print_uint_field(plist[i].priority, 10);
                shell_print_uint_field((uint32_t)plist[i].cpu_ticks, 8);
                console_write(plist[i].name);
                console_putc('\n');
            }
        }
        console_write("\n");
    } else if (memcmp(cmd, "sched", 5) == 0 && (cmd[5] == '\0' || cmd[5] == ' ')) {
        scheduler_start_demo();
    } else if (memcmp(cmd, "ipc", 3) == 0 && (cmd[3] == '\0' || cmd[3] == ' ')) {
        ipc_start_demo();
    } else if (memcmp(cmd, "market start", 12) == 0 && (cmd[12] == '\0' || cmd[12] == ' ')) {
        market_start_workload();
    } else if (memcmp(cmd, "market stop", 11) == 0 && (cmd[11] == '\0' || cmd[11] == ' ')) {
        market_stop_workload();
    } else if (memcmp(cmd, "log view", 8) == 0 && (cmd[8] == '\0' || cmd[8] == ' ')) {
        char buf[1024];
        size_t count = ledgerfs_read_logs(buf, sizeof(buf));
        console_write("\n--- LedgerFS Audit Log View (/var/log/market.log) ---\n");
        console_printf("Total Audit Records: %u\n", (uint32_t)count);
        if (count > 0) {
            console_write(buf);
        } else {
            console_write("  [No log entries recorded yet]\n");
        }
        console_write("\n");
    } else if (memcmp(cmd, "cat ", 4) == 0) {
        const char *path = cmd + 4;
        while (*path == ' ') path++;
        if (memcmp(path, "/var/log/market.log", 19) == 0 || memcmp(path, "market.log", 10) == 0) {
            char buf[1024];
            size_t count = ledgerfs_read_logs(buf, sizeof(buf));
            console_printf("\n[%s]\n", path);
            if (count > 0) {
                console_write(buf);
            } else {
                console_write("[Empty file / no logs written]\n");
            }
            console_write("\n");
        } else {
            console_printf("\n[ERR] cat: %s: No such file or directory in LedgerFS ramdisk.\nAvailable files:\n  /var/log/market.log\n\n", path);
        }
    } else if (memcmp(cmd, "spawn ", 6) == 0) {
        const char *pname = cmd + 6;
        while (*pname == ' ') pname++;
        if (*pname == '\0') {
            console_write("[ERR] Usage: spawn <process_name>\n\n");
        } else {
            pcb_t *p = process_create(pname, dummy_process_entry, 2);
            if (p) {
                console_printf("\n[SCHED] Dynamically created PCB process '%s' (PID %u)\n\n", p->name, p->pid);
            } else {
                console_write("\n[ERR] PCB table full (max 16 processes).\n\n");
            }
        }
    } else if (memcmp(cmd, "renice ", 7) == 0) {
        const char *p = cmd + 7;
        while (*p == ' ') p++;
        int pid = shell_atoi(p);
        while (*p && *p != ' ') p++;
        while (*p == ' ') p++;
        int prio = shell_atoi(p);

        if (pid < 0 || prio < 0) {
            console_write("[ERR] Usage: renice <pid> <priority (0-10)>\n\n");
        } else {
            bool ok = process_set_priority((uint32_t)pid, (uint32_t)prio);
            if (ok) {
                console_printf("\n[SCHED] Updated PID %d priority to %d. Run 'ps' to inspect.\n\n", pid, prio);
            } else {
                console_printf("\n[ERR] PID %d not found in active process table.\n\n", pid);
            }
        }
    } else if (memcmp(cmd, "alloc ", 6) == 0) {
        int bytes = shell_atoi(cmd + 6);
        if (bytes <= 0) {
            console_write("[ERR] Usage: alloc <bytes>\n\n");
        } else {
            void *ptr = kmalloc((size_t)bytes);
            if (ptr) {
                console_printf("\n[HEAP] Dynamically allocated %d bytes at address 0x%x\n", bytes, (uint32_t)(uintptr_t)ptr);
                size_t total = 0, used = 0, free_b = 0;
                heap_get_stats(&total, &used, &free_b);
                console_printf("[HEAP] Free Memory Remaining: %u KB (%u bytes)\n\n", (uint32_t)(free_b / 1024), (uint32_t)free_b);
            } else {
                console_printf("\n[ERR] kmalloc failed to allocate %d bytes (out of memory).\n\n", bytes);
            }
        }
    } else if (memcmp(cmd, "free ", 5) == 0) {
        const char *addr_str = cmd + 5;
        uintptr_t addr = shell_atou_hex(addr_str);
        if (addr == 0) {
            console_write("[ERR] Usage: free <hex_address> (e.g. free 0x123456)\n\n");
        } else {
            kfree((void *)addr);
            console_printf("\n[HEAP] Freed memory at address 0x%x & coalesced adjacent free chunks.\n", (uint32_t)addr);
            size_t total = 0, used = 0, free_b = 0;
            heap_get_stats(&total, &used, &free_b);
            console_printf("[HEAP] Free Memory Remaining: %u KB (%u bytes)\n\n", (uint32_t)(free_b / 1024), (uint32_t)free_b);
        }
    } else if (memcmp(cmd, "race demo", 9) == 0 && (cmd[9] == '\0' || cmd[9] == ' ')) {
        const uint32_t expected = RACE_DEMO_ITERATIONS * 2;
        console_write("\n============================================================\n");
        console_write("       RACE CONDITION DEMONSTRATION: LIVE COUNTER\n");
        console_write("============================================================\n");
        console_printf("[RACE] Two workers; %u increments each; expected total %u.\n",
            RACE_DEMO_ITERATIONS, expected);

        console_write("\n[1/2] UNSYNCHRONIZED: switching between counter read and write.\n");
        race_counter = 0;
        race_demo_use_lock = false;
        if (race_demo_run_workers()) {
            uint32_t unsafe_result = (uint32_t)race_counter;
            console_printf("[RACE] Unsynchronized result: %u / %u\n", unsafe_result, expected);
            if (unsafe_result < expected) {
                console_printf("[RACE] Lost updates: %u\n", expected - unsafe_result);
            } else {
                console_write("[RACE] No lost updates observed in this run.\n");
            }

            console_write("\n[2/2] SYNCHRONIZED: protecting each increment with a spinlock.\n");
            race_locked_counter = 0;
            race_demo_use_lock = true;
            spinlock_init(&race_demo_lock);
            if (race_demo_run_workers()) {
                console_printf("[RACE] Spinlock result: %u / %u\n", race_locked_counter, expected);
                if (race_locked_counter == expected) {
                    console_write("[PASS] The lock preserved every increment.\n");
                } else {
                    console_write("[FAIL] Protected counter did not reach the expected total.\n");
                }
            }
        }
        race_demo_use_lock = false;
        console_write("============================================================\n\n");
    } else if (memcmp(cmd, "msg ", 4) == 0) {
        const char *args = cmd + 4;
        while (*args == ' ') args++;
        if (*args == '\0') {
            console_write("[ERR] Usage: msg <ticker> <price>\n\n");
        } else {
            char payload[64];
            size_t i = 0;
            const char *p = args;
            while (*p && i < sizeof(payload) - 1) {
                payload[i++] = *p++;
            }
            payload[i] = '\0';

            // Extract ticker and price for real-time GUI graph updates
            char ticker[16];
            int ti = 0;
            const char *tp = args;
            while (*tp && *tp != ' ' && ti < 15) {
                ticker[ti++] = *tp++;
            }
            ticker[ti] = '\0';
            while (*tp == ' ') tp++;
            int price = shell_atoi(tp);
            if (price > 0) {
                gui_update_price(ticker, (uint32_t)price);
            }

            bool ok = ipc_send(2, 500, payload);
            if (ok) {
                console_printf("\n[IPC] Message sent to ring-buffer mailbox: '%s'\n", payload);
                console_write("[IPC] Type 'ipc' to drain and view mailbox.\n\n");
            } else {
                console_write("\n[ERR] IPC queue full.\n\n");
            }
        }
    } else if (memcmp(cmd, "order ", 6) == 0) {
        const char *p = cmd + 6;
        while (*p == ' ') p++;
        char side[8] = "BUY";
        if (memcmp(p, "buy ", 4) == 0 || memcmp(p, "BUY ", 4) == 0) {
            memcpy(side, "BUY", 4);
            p += 4;
        } else if (memcmp(p, "sell ", 5) == 0 || memcmp(p, "SELL ", 5) == 0) {
            memcpy(side, "SELL", 5);
            p += 5;
        } else {
            console_write("[ERR] Usage: order <buy|sell> <ticker> <qty> <price>\nExample: order buy NVDA 100 450\n\n");
            return;
        }
        while (*p == ' ') p++;
        char ticker[16]; int ti = 0;
        while (*p && *p != ' ' && ti < 15) ticker[ti++] = *p++;
        ticker[ti] = '\0';
        while (*p == ' ') p++;
        int qty = shell_atoi(p);
        while (*p && *p != ' ') p++;
        while (*p == ' ') p++;
        int price = shell_atoi(p);

        if (ticker[0] == '\0' || qty <= 0 || price <= 0) {
            console_write("[ERR] Usage: order <buy|sell> <ticker> <qty> <price>\nExample: order buy NVDA 100 450\n\n");
        } else {
            uint64_t cycles = 0;
            bool ok = market_execute_order(side, ticker, (uint32_t)qty, (uint32_t)price, &cycles);
            if (ok) {
                uint32_t ns = (uint32_t)((cycles * 10) / 28);
                console_printf("\n[ORDER MATCHED] %s %u %s @ $%u\n", side, qty, ticker, price);
                console_printf("[PERF] Hardware Execution Latency : %u CPU cycles (~%u nanoseconds)\n", 
                    (uint32_t)cycles, ns);
                console_printf("[AUDIT] Recorded in LedgerFS persistent log storage (/var/log/market.log)\n");
                console_printf("[GUI] Price chart updated for %s.\n\n", ticker);
            } else {
                console_write("[ERR] Failed to execute order.\n\n");
            }
        }
    } else if (memcmp(cmd, "bench", 5) == 0 && (cmd[5] == '\0' || cmd[5] == ' ')) {
        const char *p = cmd + 5;
        while (*p == ' ') p++;
        if (memcmp(p, "orders", 6) == 0) {
            p += 6;
            while (*p == ' ') p++;
        }
        int count = shell_atoi(p);
        if (count <= 0) count = 1000;
        market_run_benchmark((uint32_t)count);
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

static bool last_cursor_state = false;

static void shell_handle_char(char c, bool *has_typed) {
    if (!*has_typed) {
        console_draw_cursor(false);
        *has_typed = true;
    }

    if (c == '\n' || c == '\r') {
        console_putc('\n');
        cmd_buf[cmd_len] = '\0';
        shell_execute(cmd_buf);
        cmd_len = 0;
        memset(cmd_buf, 0, sizeof(cmd_buf));
        shell_prompt();
    } else if (c == '\b' || c == 127) {
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

void shell_init(void) {
    cmd_len = 0;
    memset(cmd_buf, 0, sizeof(cmd_buf));
    shell_prompt();
    console_draw_cursor(true);
    last_cursor_state = true;
}

void shell_update(void) {
    bool has_typed = false;

    while (keyboard_has_char()) {
        char c = keyboard_getchar();
        shell_handle_char(c, &has_typed);
    }

    while (serial_has_char()) {
        char c = serial_getchar();
        shell_handle_char(c, &has_typed);
    }

    // Perform timer-requested task switches after the IRQ has returned.
    scheduler_poll();

    // Blinking cursor toggle every ~30 ticks (300ms)
    bool current_cursor = ((timer_get_ticks() / 30) % 2) == 0;
    if (current_cursor != last_cursor_state || has_typed) {
        console_draw_cursor(current_cursor);
        last_cursor_state = current_cursor;
    }
}
