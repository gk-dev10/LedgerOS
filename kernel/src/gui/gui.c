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

#define PRICE_HISTORY_CAPACITY 35
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
    gfx_draw_string(16, 10, "LEDGEROS DASHBOARD v1.0 | PREEMPTIVE KERNEL", 0x0000FFFF, 0xFFFFFFFF);
    
    char uptime_str[32];
    uint64_t ticks = timer_get_ticks();
    uint32_t secs = (uint32_t)(ticks / 100);
    
    uptime_str[0] = 'U'; uptime_str[1] = 'p'; uptime_str[2] = ':'; uptime_str[3] = ' ';
    size_t up_pos = 4;
    uint32_t tmp_s = secs;
    char snum[16];
    int si = 0;
    if (tmp_s == 0) snum[si++] = '0';
    while (tmp_s > 0) { snum[si++] = '0' + (tmp_s % 10); tmp_s /= 10; }
    while (si > 0) uptime_str[up_pos++] = snum[--si];
    uptime_str[up_pos++] = 's'; uptime_str[up_pos] = '\0';

    gfx_draw_string(fb_w - 180, 10, uptime_str, 0x0000FF00, 0xFFFFFFFF);
    gfx_draw_rect_outline(0, 0, fb_w, 36, 0x0000FFFF);

    // 2. Window 1 (Top Left): CPU Scheduler & Task Monitor
    size_t w1_x = 16, w1_y = 50, w1_w = (fb_w / 2) - 24, w1_h = 240;
    gfx_draw_rect(w1_x, w1_y, w1_w, w1_h, 0x00141824);
    gfx_draw_rect_outline(w1_x, w1_y, w1_w, w1_h, 0x003A4560);
    gfx_draw_rect(w1_x, w1_y, w1_w, 24, 0x00222A40);
    gfx_draw_string(w1_x + 8, w1_y + 4, "[1] CPU TASK MONITOR", 0x00FFFF00, 0xFFFFFFFF);

    pcb_t *plist = NULL;
    size_t count = process_get_all(&plist);
    
    // Column Headers (at 16px char width)
    gfx_draw_string(w1_x + 8,   w1_y + 32, "PID", 0x00A0A0B0, 0xFFFFFFFF);
    gfx_draw_string(w1_x + 64,  w1_y + 32, "NAME", 0x00A0A0B0, 0xFFFFFFFF);
    gfx_draw_string(w1_x + 240, w1_y + 32, "STATE", 0x00A0A0B0, 0xFFFFFFFF);

    size_t row_y = w1_y + 54;
    int displayed = 0;
    for (size_t i = 0; i < count && displayed < 7; i++) {
        if (plist[i].state != PROCESS_UNUSED) {
            char pid_buf[8];
            pid_buf[0] = '0' + (plist[i].pid % 10);
            pid_buf[1] = '\0';

            char name_buf[12];
            size_t k = 0;
            for (; k < 10 && plist[i].name[k] != '\0'; k++) {
                name_buf[k] = plist[i].name[k];
            }
            name_buf[k] = '\0';

            const char *st_str = "READY";
            uint32_t st_color = 0x0000FF00;

            switch (plist[i].state) {
                case PROCESS_RUNNING: st_str = "RUNNING"; st_color = 0x0000FFFF; break;
                case PROCESS_BLOCKED: st_str = "BLOCKED"; st_color = 0x00FF8C00; break;
                case PROCESS_TERMINATED: st_str = "TERM"; st_color = 0x00808080; break;
                default: break;
            }

            gfx_draw_string(w1_x + 8,   row_y, pid_buf,  0x00FFFFFF, 0xFFFFFFFF);
            gfx_draw_string(w1_x + 64,  row_y, name_buf, 0x00FFFFFF, 0xFFFFFFFF);
            gfx_draw_string(w1_x + 240, row_y, st_str,   st_color,   0xFFFFFFFF);

            row_y += 24;
            displayed++;
        }
    }

    // 3. Window 2 (Top Right): Memory Allocation Gauges
    size_t w2_x = (fb_w / 2) + 8, w2_y = 50, w2_w = (fb_w / 2) - 24, w2_h = 240;
    gfx_draw_rect(w2_x, w2_y, w2_w, w2_h, 0x00141824);
    gfx_draw_rect_outline(w2_x, w2_y, w2_w, w2_h, 0x003A4560);
    gfx_draw_rect(w2_x, w2_y, w2_w, 24, 0x00222A40);
    gfx_draw_string(w2_x + 8, w2_y + 4, "[2] MEMORY & PAGING GAUGES", 0x0000FFFF, 0xFFFFFFFF);

    size_t h_total = 0, h_used = 0, h_free = 0;
    heap_get_stats(&h_total, &h_used, &h_free);
    uint32_t heap_pct = (uint32_t)((h_used * 100) / h_total);

    gfx_draw_string(w2_x + 12, w2_y + 36, "Kernel Heap (4MB):", 0x00E0E0E0, 0xFFFFFFFF);
    gfx_draw_gauge(w2_x + 12, w2_y + 54, w2_w - 24, 18, heap_pct, 0x0000C8FF, 0x00202535);

    size_t p_total = 0, p_used = 0, p_free = 0;
    pmm_get_stats(&p_total, &p_used, &p_free);
    uint32_t pmm_pct = (uint32_t)((p_used * 100) / p_total);

    gfx_draw_string(w2_x + 12, w2_y + 84, "PMM 4KB Page Frames:", 0x00E0E0E0, 0xFFFFFFFF);
    gfx_draw_gauge(w2_x + 12, w2_y + 102, w2_w - 24, 18, pmm_pct, 0x0000FF7F, 0x00202535);

    gfx_draw_string(w2_x + 12, w2_y + 134, "VMM: 4-Level Paging Active", 0x0000FF00, 0xFFFFFFFF);
    gfx_draw_string(w2_x + 12, w2_y + 158, "Syscall: MSR LSTAR Vector", 0x0000FF00, 0xFFFFFFFF);

    // 4. Window 3 (Bottom Left): IPC Mailbox & Storage Audit
    size_t w3_x = 16, w3_y = 300, w3_w = (fb_w / 2) - 24, w3_h = 240;
    gfx_draw_rect(w3_x, w3_y, w3_w, w3_h, 0x00141824);
    gfx_draw_rect_outline(w3_x, w3_y, w3_w, w3_h, 0x003A4560);
    gfx_draw_rect(w3_x, w3_y, w3_w, 24, 0x00222A40);
    gfx_draw_string(w3_x + 8, w3_y + 4, "[3] IPC MAILBOX & LEDGERFS", 0x00FF00FF, 0xFFFFFFFF);

    gfx_draw_string(w3_x + 12, w3_y + 36, "IPC Semaphore Queue:", 0x00E0E0E0, 0xFFFFFFFF);
    gfx_draw_gauge(w3_x + 12, w3_y + 54, w3_w - 24, 18, 25, 0x00FFD700, 0x00202535);

    size_t logs_count = ledgerfs_get_count();
    gfx_draw_string(w3_x + 12, w3_y + 84, "LedgerFS Ramdisk Storage:", 0x00E0E0E0, 0xFFFFFFFF);
    
    char log_str[32];
    log_str[0] = 'L'; log_str[1] = 'o'; log_str[2] = 'g'; log_str[3] = ' '; log_str[4] = 'C'; log_str[5] = 'o'; log_str[6] = 'u'; log_str[7] = 'n'; log_str[8] = 't'; log_str[9] = ':'; log_str[10] = ' ';
    size_t lp = 11;
    uint32_t tmp_l = (uint32_t)logs_count;
    char lnum[16];
    int li = 0;
    if (tmp_l == 0) lnum[li++] = '0';
    while (tmp_l > 0) { lnum[li++] = '0' + (tmp_l % 10); tmp_l /= 10; }
    while (li > 0) log_str[lp++] = lnum[--li];
    log_str[lp] = '\0';

    gfx_draw_string(w3_x + 12, w3_y + 106, log_str, 0x0000FF00, 0xFFFFFFFF);
    gfx_draw_string(w3_x + 12, w3_y + 134, "Sync: Mutex/Sem/CondVar", 0x0000FFFF, 0xFFFFFFFF);

    // 5. Window 4 (Bottom Right): Live Market Price & Volatility Chart
    size_t w4_x = (fb_w / 2) + 8, w4_y = 300, w4_w = (fb_w / 2) - 24, w4_h = 240;
    gfx_draw_rect(w4_x, w4_y, w4_w, w4_h, 0x00141824);
    gfx_draw_rect_outline(w4_x, w4_y, w4_w, w4_h, 0x003A4560);
    gfx_draw_rect(w4_x, w4_y, w4_w, 24, 0x00222A40);
    gfx_draw_string(w4_x + 8, w4_y + 4, "[4] LIVE MARKET CHART", 0x0000FF00, 0xFFFFFFFF);

    // Render price line chart
    size_t chart_x = w4_x + 16;
    size_t chart_y = w4_y + 40;
    size_t chart_w = w4_w - 32;
    size_t chart_h = 170;

    gfx_draw_rect(chart_x, chart_y, chart_w, chart_h, 0x000A0C12);
    gfx_draw_rect_outline(chart_x, chart_y, chart_w, chart_h, 0x00202838);

    if (price_count > 1) {
        size_t step_x = chart_w / PRICE_HISTORY_CAPACITY;
        for (size_t i = 0; i < price_count - 1; i++) {
            size_t px1 = chart_x + (i * step_x);
            size_t py1 = chart_y + chart_h - 15 - ((price_history[i] % 130));

            size_t px2 = chart_x + ((i + 1) * step_x);
            size_t py2 = chart_y + chart_h - 15 - ((price_history[i + 1] % 130));

            gfx_draw_line(px1, py1, px2, py2, 0x0000FF00); // Bright green trend line
        }
    }
}
