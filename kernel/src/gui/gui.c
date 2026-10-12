#include "gui.h"
#include "gfx.h"
#include "../drivers/framebuffer.h"
#include "../drivers/keyboard.h"
#include "../drivers/serial.h"
#include "../process/process.h"
#include "../scheduler/scheduler.h"
#include "../memory/heap.h"
#include "../memory/pmm.h"
#include "../fs/ledgerfs.h"
#include "../timer/timer.h"
#include "../memory.h"
#include "../console/console.h"

#define PRICE_HISTORY_CAPACITY 30
static uint32_t price_history[PRICE_HISTORY_CAPACITY];
static size_t price_count = 0;

static char last_ticker[16] = "NVDA";
static uint32_t last_price = 350;
static bool last_is_spike = false;

void gui_init(void) {
    price_count = 0;
    memset(price_history, 0, sizeof(price_history));
    // Seed initial price history curve
    for (int i = 0; i < 15; i++) {
        price_history[i] = 200 + (i * 13) % 80;
        price_count++;
    }
    memcpy(last_ticker, "NVDA", 5);
    last_price = 350;
    last_is_spike = false;
}

void gui_update_price(const char *ticker, uint32_t price) {
    if (ticker) {
        size_t len = 0;
        while (ticker[len] && len < 15) {
            last_ticker[len] = ticker[len];
            len++;
        }
        last_ticker[len] = '\0';
    }

    last_price = price;
    last_is_spike = (price > 500);

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

static void render_number(char *buf, size_t *pos, uint32_t num) {
    char snum[16];
    int si = 0;
    if (num == 0) snum[si++] = '0';
    while (num > 0) { snum[si++] = '0' + (num % 10); num /= 10; }
    while (si > 0) buf[(*pos)++] = snum[--si];
}

void gui_render_dashboard(void) {
    uint32_t fb_w = framebuffer_get_width();
    uint32_t fb_h = framebuffer_get_height();

    // Dark theme background
    gfx_draw_rect(0, 0, fb_w, fb_h, 0x000C0E17);

    // 1. Top Header Banner
    gfx_draw_rect(0, 0, fb_w, 36, 0x00161C2E);
    gfx_draw_string(16, 10, "LEDGEROS REAL-TIME DASHBOARD", 0x0000FFFF, 0xFFFFFFFF);
    
    char uptime_str[32];
    uint64_t ticks = timer_get_ticks();
    uint32_t secs = (uint32_t)(ticks / 100);
    
    uptime_str[0] = 'U'; uptime_str[1] = 'p'; uptime_str[2] = ':'; uptime_str[3] = ' ';
    size_t up_pos = 4;
    render_number(uptime_str, &up_pos, secs);
    uptime_str[up_pos++] = 's'; uptime_str[up_pos] = '\0';

    gfx_draw_string(fb_w - 480, 10, "[Press ESC or 'q' to Exit]", 0x00FFBB00, 0xFFFFFFFF);
    gfx_draw_string(fb_w - 180, 10, uptime_str, 0x0000FF00, 0xFFFFFFFF);
    gfx_draw_rect_outline(0, 0, fb_w, 36, 0x0000FFFF);

    // 2. Window 1 (Top Left): Real-Time CPU Task Monitor
    size_t w1_x = 16, w1_y = 48, w1_w = (fb_w / 2) - 24, w1_h = 245;
    gfx_draw_rect(w1_x, w1_y, w1_w, w1_h, 0x00121622);
    gfx_draw_rect_outline(w1_x, w1_y, w1_w, w1_h, 0x002E3850);
    gfx_draw_rect(w1_x, w1_y, w1_w, 24, 0x001E263C);
    gfx_draw_string(w1_x + 8, w1_y + 4, "[1] REAL-TIME CPU TASK MONITOR", 0x00FFFF00, 0xFFFFFFFF);

    pcb_t *plist = NULL;
    size_t count = process_get_all(&plist);
    
    // Column Headers
    gfx_draw_string(w1_x + 8,   w1_y + 32, "PID", 0x00A0A0B0, 0xFFFFFFFF);
    gfx_draw_string(w1_x + 64,  w1_y + 32, "TASK NAME", 0x00A0A0B0, 0xFFFFFFFF);
    gfx_draw_string(w1_x + 240, w1_y + 32, "STATUS", 0x00A0A0B0, 0xFFFFFFFF);
    gfx_draw_string(w1_x + 360, w1_y + 32, "TIME", 0x00A0A0B0, 0xFFFFFFFF);

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

            char time_buf[16];
            size_t tp = 0;
            uint32_t task_secs = (uint32_t)(plist[i].cpu_ticks / 100);
            render_number(time_buf, &tp, task_secs);
            time_buf[tp++] = 's';
            time_buf[tp] = '\0';

            gfx_draw_string(w1_x + 8,   row_y, pid_buf,  0x00FFFFFF, 0xFFFFFFFF);
            gfx_draw_string(w1_x + 64,  row_y, name_buf, 0x00FFFFFF, 0xFFFFFFFF);
            gfx_draw_string(w1_x + 240, row_y, st_str,   st_color,   0xFFFFFFFF);
            gfx_draw_string(w1_x + 360, row_y, time_buf, 0x00D0D0D0, 0xFFFFFFFF);

            row_y += 24;
            displayed++;
        }
    }

    // 3. Window 2 (Top Right): Live Memory & Paging Metrics
    size_t w2_x = (fb_w / 2) + 8, w2_y = 48, w2_w = (fb_w / 2) - 24, w2_h = 245;
    gfx_draw_rect(w2_x, w2_y, w2_w, w2_h, 0x00121622);
    gfx_draw_rect_outline(w2_x, w2_y, w2_w, w2_h, 0x002E3850);
    gfx_draw_rect(w2_x, w2_y, w2_w, 24, 0x001E263C);
    gfx_draw_string(w2_x + 8, w2_y + 4, "[2] LIVE MEMORY & PAGING METRICS", 0x0000FFFF, 0xFFFFFFFF);

    size_t h_total = 0, h_used = 0, h_free = 0;
    heap_get_stats(&h_total, &h_used, &h_free);
    uint32_t heap_pct = (uint32_t)((h_used * 100) / h_total);

    char heap_lbl[48];
    size_t hlp = 0;
    heap_lbl[hlp++] = 'H'; heap_lbl[hlp++] = 'e'; heap_lbl[hlp++] = 'a'; heap_lbl[hlp++] = 'p'; heap_lbl[hlp++] = ':'; heap_lbl[hlp++] = ' ';
    render_number(heap_lbl, &hlp, heap_pct);
    heap_lbl[hlp++] = '%'; heap_lbl[hlp++] = ' '; heap_lbl[hlp++] = '(';
    render_number(heap_lbl, &hlp, (uint32_t)(h_used / 1024));
    heap_lbl[hlp++] = 'K'; heap_lbl[hlp++] = 'B'; heap_lbl[hlp++] = '/';
    render_number(heap_lbl, &hlp, (uint32_t)(h_total / 1024));
    heap_lbl[hlp++] = 'K'; heap_lbl[hlp++] = 'B'; heap_lbl[hlp++] = ')'; heap_lbl[hlp] = '\0';

    gfx_draw_string(w2_x + 12, w2_y + 36, heap_lbl, 0x00E0E0E0, 0xFFFFFFFF);
    gfx_draw_gauge(w2_x + 12, w2_y + 56, w2_w - 24, 18, heap_pct, 0x0000C8FF, 0x00202535);

    size_t p_total = 0, p_used = 0, p_free = 0;
    pmm_get_stats(&p_total, &p_used, &p_free);
    uint32_t pmm_pct = (uint32_t)((p_used * 100) / p_total);

    char pmm_lbl[48];
    size_t plp = 0;
    pmm_lbl[plp++] = 'P'; pmm_lbl[plp++] = 'M'; pmm_lbl[plp++] = 'M'; pmm_lbl[plp++] = ':'; pmm_lbl[plp++] = ' ';
    render_number(pmm_lbl, &plp, pmm_pct);
    pmm_lbl[plp++] = '%'; pmm_lbl[plp++] = ' '; pmm_lbl[plp++] = '(';
    render_number(pmm_lbl, &plp, (uint32_t)p_used);
    pmm_lbl[plp++] = '/';
    render_number(pmm_lbl, &plp, (uint32_t)p_total);
    pmm_lbl[plp++] = ' '; pmm_lbl[plp++] = 'P'; pmm_lbl[plp++] = 'g'; pmm_lbl[plp++] = 's'; pmm_lbl[plp++] = ')'; pmm_lbl[plp] = '\0';

    gfx_draw_string(w2_x + 12, w2_y + 86, pmm_lbl, 0x00E0E0E0, 0xFFFFFFFF);
    gfx_draw_gauge(w2_x + 12, w2_y + 106, w2_w - 24, 18, pmm_pct, 0x0000FF7F, 0x00202535);

    gfx_draw_string(w2_x + 12, w2_y + 138, "VMM: 4-Level Paging (PML4)", 0x0000FF00, 0xFFFFFFFF);
    gfx_draw_string(w2_x + 12, w2_y + 162, "Syscall: MSR (LSTAR Vector)", 0x0000FF00, 0xFFFFFFFF);

    // 4. Window 3 (Bottom Left): IPC Mailbox & Storage Audit
    size_t w3_x = 16, w3_y = 300, w3_w = (fb_w / 2) - 24, w3_h = 245;
    gfx_draw_rect(w3_x, w3_y, w3_w, w3_h, 0x00121622);
    gfx_draw_rect_outline(w3_x, w3_y, w3_w, w3_h, 0x002E3850);
    gfx_draw_rect(w3_x, w3_y, w3_w, 24, 0x001E263C);
    gfx_draw_string(w3_x + 8, w3_y + 4, "[3] IPC MAILBOX & AUDIT STORAGE", 0x00FF00FF, 0xFFFFFFFF);

    gfx_draw_string(w3_x + 12, w3_y + 36, "IPC Semaphore Queue (16 Max):", 0x00E0E0E0, 0xFFFFFFFF);
    gfx_draw_gauge(w3_x + 12, w3_y + 56, w3_w - 24, 18, 25, 0x00FFD700, 0x00202535);

    size_t logs_count = ledgerfs_get_count();
    char log_str[48];
    size_t lp = 0;
    log_str[lp++] = 'L'; log_str[lp++] = 'e'; log_str[lp++] = 'd'; log_str[lp++] = 'g'; log_str[lp++] = 'e'; log_str[lp++] = 'r'; log_str[lp++] = 'F'; log_str[lp++] = 'S'; log_str[lp++] = ' '; log_str[lp++] = 'L'; log_str[lp++] = 'o'; log_str[lp++] = 'g'; log_str[lp++] = 's'; log_str[lp++] = ':'; log_str[lp++] = ' ';
    render_number(log_str, &lp, (uint32_t)logs_count);
    log_str[lp++] = ' '; log_str[lp++] = 'R'; log_str[lp++] = 'e'; log_str[lp++] = 'c'; log_str[lp++] = 's'; log_str[lp] = '\0';

    gfx_draw_string(w3_x + 12, w3_y + 86, log_str, 0x0000FF00, 0xFFFFFFFF);
    gfx_draw_string(w3_x + 12, w3_y + 116, "Sync Primitives Active:", 0x00E0E0E0, 0xFFFFFFFF);
    gfx_draw_string(w3_x + 12, w3_y + 138, "Mutexes | Semaphores | CondVars", 0x0000FFFF, 0xFFFFFFFF);

    // 5. Window 4 (Bottom Right): Live Market Price & Volatility Chart
    size_t w4_x = (fb_w / 2) + 8, w4_y = 300, w4_w = (fb_w / 2) - 24, w4_h = 245;
    gfx_draw_rect(w4_x, w4_y, w4_w, w4_h, 0x00121622);
    gfx_draw_rect_outline(w4_x, w4_y, w4_w, w4_h, 0x002E3850);
    gfx_draw_rect(w4_x, w4_y, w4_w, 24, 0x001E263C);
    gfx_draw_string(w4_x + 8, w4_y + 4, "[4] LIVE MARKET TICKER CHART", 0x0000FF00, 0xFFFFFFFF);

    // Render price line chart
    size_t chart_x = w4_x + 16;
    size_t chart_y = w4_y + 40;
    size_t chart_w = w4_w - 32;
    size_t chart_h = 145;

    gfx_draw_rect(chart_x, chart_y, chart_w, chart_h, 0x000A0C12);
    gfx_draw_rect_outline(chart_x, chart_y, chart_w, chart_h, 0x00202838);

    if (price_count > 1) {
        size_t step_x = chart_w / PRICE_HISTORY_CAPACITY;
        for (size_t i = 0; i < price_count - 1; i++) {
            size_t px1 = chart_x + (i * step_x);
            size_t py1 = chart_y + chart_h - 15 - ((price_history[i] % 110));

            size_t px2 = chart_x + ((i + 1) * step_x);
            size_t py2 = chart_y + chart_h - 15 - ((price_history[i + 1] % 110));

            gfx_draw_line(px1, py1, px2, py2, 0x0000FF00); // Bright green trend line
        }
    }

    // Ticker banner below chart
    char tick_banner[48];
    size_t tbp = 0;
    tick_banner[tbp++] = 'T'; tick_banner[tbp++] = 'I'; tick_banner[tbp++] = 'C'; tick_banner[tbp++] = 'K'; tick_banner[tbp++] = ':'; tick_banner[tbp++] = ' ';
    size_t tk = 0;
    while (last_ticker[tk] && tk < 8) tick_banner[tbp++] = last_ticker[tk++];
    tick_banner[tbp++] = ' '; tick_banner[tbp++] = '='; tick_banner[tbp++] = ' '; tick_banner[tbp++] = '$';
    render_number(tick_banner, &tbp, last_price);
    tick_banner[tbp] = '\0';

    gfx_draw_string(w4_x + 16, w4_y + 195, tick_banner, 0x00FFFF00, 0xFFFFFFFF);

    if (last_is_spike) {
        gfx_draw_string(w4_x + 220, w4_y + 195, "[SPIKE ALERT]", 0x00FF0000, 0xFFFFFFFF);
    } else {
        gfx_draw_string(w4_x + 220, w4_y + 195, "[NORMAL]", 0x0000FF00, 0xFFFFFFFF);
    }
}

void gui_run_live_mode(void) {
    // Flush any leftover key in keyboard and serial buffers (e.g. Enter pressed to launch gui)
    while (keyboard_has_char()) {
        (void)keyboard_getchar();
    }
    while (serial_has_char()) {
        (void)serial_getchar();
    }

    // Immediately render dashboard frame on entry
    gui_render_dashboard();

    uint64_t start_time = timer_get_ticks();
    uint64_t last_draw_ticks = start_time;

    for (;;) {
        uint64_t current_ticks = timer_get_ticks();
        if (current_ticks - last_draw_ticks >= 10) { // Redraw every 10 ticks (100ms)
            last_draw_ticks = current_ticks;
            gui_render_dashboard();
        }

        // Allow exit via keyboard or serial terminal
        if (current_ticks - start_time > 15) { // 150ms debounce
            bool should_exit = false;
            if (keyboard_has_char()) {
                char k = keyboard_getchar();
                if (k == 27 || k == 'q' || k == 'Q' || k == '\n' || k == ' ') {
                    should_exit = true;
                }
            }
            if (serial_has_char()) {
                char s = serial_getchar();
                if (s == 27 || s == 'q' || s == 'Q' || s == '\n' || s == ' ') {
                    should_exit = true;
                }
            }

            if (should_exit) {
                console_clear();
                console_write("=========================================\n");
                console_write("              LEDGEROS SHELL\n");
                console_write("=========================================\n\n");
                console_set_color(0x0000FF00, 0x00101018);
                console_write("LedgerOS> ");
                console_set_color(0x00FFFFFF, 0x00101018);
                return;
            }
        }

        asm volatile("pause");
    }
}
