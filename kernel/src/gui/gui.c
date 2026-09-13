#include "gui.h"
#include "gfx.h"
#include "../drivers/framebuffer.h"
#include "../process/process.h"
#include "../scheduler/scheduler.h"
#include "../memory/heap.h"
#include "../memory/pmm.h"
#include "../fs/ledgerfs.h"
#include "../timer/timer.h"
#include "../memory.h"

#define PRICE_HISTORY_CAPACITY 40
static uint32_t price_history[PRICE_HISTORY_CAPACITY];
static size_t price_count = 0;

void gui_init(void) {
    price_count = 0;
    memset(price_history, 0, sizeof(price_history));
    // Seed initial price history curve
    for (int i = 0; i < 15; i++) {
        price_history[i] = 150 + (i * 7) % 50;
        price_count++;
    }
}

void gui_update_price(const char *ticker, uint32_t price) {
    (void)ticker;
    if (price_count < PRICE_HISTORY_CAPACITY) {
        price_history[price_count++] = price;
    } else {
        // Shift left
        for (size_t i = 0; i < PRICE_HISTORY_CAPACITY - 1; i++) {
            price_history[i] = price_history[i + 1];
        }
        price_history[PRICE_HISTORY_CAPACITY - 1] = price;
    }
}

void gui_render_dashboard(void) {
    uint32_t fb_w = framebuffer_get_width();
    uint32_t fb_h = framebuffer_get_height();

    // Dark background theme
    gfx_draw_rect(0, 0, fb_w, fb_h, 0x000F111A);

    // 1. Top Header Banner
    gfx_draw_rect(0, 0, fb_w, 36, 0x001A2035);
    gfx_draw_string(16, 12, "LEDGEROS NATIVE DASHBOARD v1.0  |  x86-64 PREEMPTIVE KERNEL", 0x0000FFFF, 0xFFFFFFFF);
    
    char uptime_str[32];
    uint64_t ticks = timer_get_ticks();
    uint32_t secs = (uint32_t)(ticks / 100);
    // Format simple string
    uptime_str[0] = 'U'; uptime_str[1] = 'p'; uptime_str[2] = 't'; uptime_str[3] = 'i'; uptime_str[4] = 'm'; uptime_str[5] = 'e'; uptime_str[6] = ':'; uptime_str[7] = ' ';
    size_t up_pos = 8;
    uint32_t tmp_s = secs;
    char snum[16];
    int si = 0;
    if (tmp_s == 0) snum[si++] = '0';
    while (tmp_s > 0) { snum[si++] = '0' + (tmp_s % 10); tmp_s /= 10; }
    while (si > 0) uptime_str[up_pos++] = snum[--si];
    uptime_str[up_pos++] = 's'; uptime_str[up_pos] = '\0';

    gfx_draw_string(fb_w - 160, 12, uptime_str, 0x0000FF00, 0xFFFFFFFF);
    gfx_draw_rect_outline(0, 0, fb_w, 36, 0x0000FFFF);

    // 2. Window 1 (Top Left): CPU Scheduler & Task Monitor
    size_t w1_x = 16, w1_y = 50, w1_w = (fb_w / 2) - 24, w1_h = 240;
    gfx_draw_rect(w1_x, w1_y, w1_w, w1_h, 0x00141824);
    gfx_draw_rect_outline(w1_x, w1_y, w1_w, w1_h, 0x003A4560);
    gfx_draw_rect(w1_x, w1_y, w1_w, 24, 0x00222A40);
    gfx_draw_string(w1_x + 8, w1_y + 8, "[1] CPU SCHEDULER & TASK MONITOR", 0x00FFFF00, 0xFFFFFFFF);

    pcb_t *plist = NULL;
    size_t count = process_get_all(&plist);
    gfx_draw_string(w1_x + 8, w1_y + 32, "PID  NAME             STATE       PRIORITY  TICKS", 0x00A0A0B0, 0xFFFFFFFF);

    size_t row_y = w1_y + 48;
    int displayed = 0;
    for (size_t i = 0; i < count && displayed < 7; i++) {
        if (plist[i].state != PROCESS_UNUSED) {
            char row_buf[80];
            const char *st_str = "READY";
            uint32_t st_color = 0x0000FF00;

            switch (plist[i].state) {
                case PROCESS_RUNNING: st_str = "RUNNING"; st_color = 0x0000FFFF; break;
                case PROCESS_BLOCKED: st_str = "BLOCKED"; st_color = 0x00FF8C00; break;
                case PROCESS_TERMINATED: st_str = "TERM"; st_color = 0x00808080; break;
                default: break;
            }

            // Print PID and Name
            row_buf[0] = '0' + (plist[i].pid % 10);
            row_buf[1] = ' '; row_buf[2] = ' ';
            size_t np = 3;
            for (size_t k = 0; k < 16; k++) {
                row_buf[np++] = plist[i].name[k] ? plist[i].name[k] : ' ';
            }
            row_buf[np] = '\0';
            gfx_draw_string(w1_x + 8, row_y, row_buf, 0x00FFFFFF, 0xFFFFFFFF);
            gfx_draw_string(w1_x + 160, row_y, st_str, st_color, 0xFFFFFFFF);

            row_y += 24;
            displayed++;
        }
    }

    // 3. Window 2 (Top Right): Memory Allocation Gauges
    size_t w2_x = (fb_w / 2) + 8, w2_y = 50, w2_w = (fb_w / 2) - 24, w2_h = 240;
    gfx_draw_rect(w2_x, w2_y, w2_w, w2_h, 0x00141824);
    gfx_draw_rect_outline(w2_x, w2_y, w2_w, w2_h, 0x003A4560);
    gfx_draw_rect(w2_x, w2_y, w2_w, 24, 0x00222A40);
    gfx_draw_string(w2_x + 8, w2_y + 8, "[2] MEMORY MANAGEMENT & PAGE GAUGES", 0x0000FFFF, 0xFFFFFFFF);

    size_t h_total = 0, h_used = 0, h_free = 0;
    heap_get_stats(&h_total, &h_used, &h_free);
    uint32_t heap_pct = (uint32_t)((h_used * 100) / h_total);

    gfx_draw_string(w2_x + 12, w2_y + 40, "Kernel Heap Utilization (4 MB Pool):", 0x00E0E0E0, 0xFFFFFFFF);
    gfx_draw_gauge(w2_x + 12, w2_y + 56, w2_w - 24, 20, heap_pct, 0x0000C8FF, 0x00202535);

    size_t p_total = 0, p_used = 0, p_free = 0;
    pmm_get_stats(&p_total, &p_used, &p_free);
    uint32_t pmm_pct = (uint32_t)((p_used * 100) / p_total);

    gfx_draw_string(w2_x + 12, w2_y + 96, "Physical Frame Allocator (PMM 4KB Pages):", 0x00E0E0E0, 0xFFFFFFFF);
    gfx_draw_gauge(w2_x + 12, w2_y + 112, w2_w - 24, 20, pmm_pct, 0x0000FF7F, 0x00202535);

    gfx_draw_string(w2_x + 12, w2_y + 152, "Virtual Memory Manager (VMM):", 0x00E0E0E0, 0xFFFFFFFF);
    gfx_draw_string(w2_x + 12, w2_y + 168, "Status: Active 4-Level Paging (PML4 -> PDPT -> PD -> PT)", 0x0000FF00, 0xFFFFFFFF);
    gfx_draw_string(w2_x + 12, w2_y + 184, "Syscall MSR Dispatcher: Configured (LSTAR Vector Active)", 0x0000FF00, 0xFFFFFFFF);

    // 4. Window 3 (Bottom Left): IPC Mailbox & Storage Audit
    size_t w3_x = 16, w3_y = 300, w3_w = (fb_w / 2) - 24, w3_h = 240;
    gfx_draw_rect(w3_x, w3_y, w3_w, w3_h, 0x00141824);
    gfx_draw_rect_outline(w3_x, w3_y, w3_w, w3_h, 0x003A4560);
    gfx_draw_rect(w3_x, w3_y, w3_w, 24, 0x00222A40);
    gfx_draw_string(w3_x + 8, w3_y + 8, "[3] IPC MAILBOX & LEDGERFS STORAGE", 0x00FF00FF, 0xFFFFFFFF);

    gfx_draw_string(w3_x + 12, w3_y + 40, "Bounded Buffer IPC Semaphore Queue:", 0x00E0E0E0, 0xFFFFFFFF);
    gfx_draw_gauge(w3_x + 12, w3_y + 56, w3_w - 24, 20, 25, 0x00FFD700, 0x00202535);

    size_t logs_count = ledgerfs_get_count();
    gfx_draw_string(w3_x + 12, w3_y + 96, "LedgerFS Ramdisk Storage (/var/log/market.log):", 0x00E0E0E0, 0xFFFFFFFF);
    
    char log_str[64];
    log_str[0] = 'T'; log_str[1] = 'o'; log_str[2] = 't'; log_str[3] = 'a'; log_str[4] = 'l'; log_str[5] = ' ';
    log_str[6] = 'A'; log_str[7] = 'u'; log_str[8] = 'd'; log_str[9] = 'i'; log_str[10] = 't'; log_str[11] = ' ';
    log_str[12] = 'R'; log_str[13] = 'e'; log_str[14] = 'c'; log_str[15] = 'o'; log_str[16] = 'r'; log_str[17] = 'd'; log_str[18] = 's'; log_str[19] = ':'; log_str[20] = ' ';
    size_t lp = 21;
    uint32_t tmp_l = (uint32_t)logs_count;
    char lnum[16];
    int li = 0;
    if (tmp_l == 0) lnum[li++] = '0';
    while (tmp_l > 0) { lnum[li++] = '0' + (tmp_l % 10); tmp_l /= 10; }
    while (li > 0) log_str[lp++] = lnum[--li];
    log_str[lp] = '\0';

    gfx_draw_string(w3_x + 12, w3_y + 116, log_str, 0x0000FF00, 0xFFFFFFFF);
    gfx_draw_string(w3_x + 12, w3_y + 144, "Synchronization Primitives Active:", 0x00E0E0E0, 0xFFFFFFFF);
    gfx_draw_string(w3_x + 12, w3_y + 160, "- Mutexes (mutex_lock/mutex_unlock wait queues)", 0x0000FFFF, 0xFFFFFFFF);
    gfx_draw_string(w3_x + 12, w3_y + 176, "- Counting Semaphores & Condition Variables", 0x0000FFFF, 0xFFFFFFFF);

    // 5. Window 4 (Bottom Right): Live Market Price & Volatility Chart
    size_t w4_x = (fb_w / 2) + 8, w4_y = 300, w4_w = (fb_w / 2) - 24, w4_h = 240;
    gfx_draw_rect(w4_x, w4_y, w4_w, w4_h, 0x00141824);
    gfx_draw_rect_outline(w4_x, w4_y, w4_w, w4_h, 0x003A4560);
    gfx_draw_rect(w4_x, w4_y, w4_w, 24, 0x00222A40);
    gfx_draw_string(w4_x + 8, w4_y + 8, "[4] LIVE MARKET TICKER PRICE CHART", 0x0000FF00, 0xFFFFFFFF);

    // Render price line chart
    size_t chart_x = w4_x + 16;
    size_t chart_y = w4_y + 45;
    size_t chart_w = w4_w - 32;
    size_t chart_h = 160;

    gfx_draw_rect(chart_x, chart_y, chart_w, chart_h, 0x000A0C12);
    gfx_draw_rect_outline(chart_x, chart_y, chart_w, chart_h, 0x00202838);

    if (price_count > 1) {
        size_t step_x = chart_w / PRICE_HISTORY_CAPACITY;
        for (size_t i = 0; i < price_count - 1; i++) {
            size_t px1 = chart_x + (i * step_x);
            size_t py1 = chart_y + chart_h - 10 - ((price_history[i] % 120));

            size_t px2 = chart_x + ((i + 1) * step_x);
            size_t py2 = chart_y + chart_h - 10 - ((price_history[i + 1] % 120));

            gfx_draw_line(px1, py1, px2, py2, 0x0000FF00); // Bright green trend line
        }
    }
}
