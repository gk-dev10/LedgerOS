#include "market.h"
#include "../process/process.h"
#include "../scheduler/scheduler.h"
#include "../ipc/ipc.h"
#include "../console/console.h"
#include "../timer/timer.h"
#include "../sync/mutex.h"

static bool market_running = false;
static pcb_t *producer_pcb = NULL;
static pcb_t *matching_pcb = NULL;
static pcb_t *alert_pcb = NULL;
static pcb_t *logger_pcb = NULL;

static void market_feed_producer_task(void) {
    const char *tickers[] = {"NVDA", "BTC", "AAPL", "TSLA", "ETH"};
    uint32_t seq = 1;

    while (market_running) {
        const char *t = tickers[seq % 5];
        uint32_t price = 150 + (seq * 37) % 800;
        bool is_volatility_spike = (seq % 4 == 0);

        char payload[64];
        // Format payload string manually
        payload[0] = t[0]; payload[1] = t[1]; payload[2] = t[2]; payload[3] = t[3] ? t[3] : '\0';
        size_t pos = t[3] ? 4 : 3;
        payload[pos++] = ':'; payload[pos++] = ' '; payload[pos++] = '$';

        uint32_t tmp = price;
        char pnum[16];
        int pi = 0;
        if (tmp == 0) pnum[pi++] = '0';
        while (tmp > 0) { pnum[pi++] = '0' + (tmp % 10); tmp /= 10; }
        while (pi > 0) payload[pos++] = pnum[--pi];
        
        if (is_volatility_spike) {
            payload[pos++] = ' '; payload[pos++] = '['; payload[pos++] = 'S';
            payload[pos++] = 'P'; payload[pos++] = 'I'; payload[pos++] = 'K'; payload[pos++] = 'E'; payload[pos++] = ']';
        }
        payload[pos] = '\0';

        uint32_t msg_type = is_volatility_spike ? 999 : 100; // Type 999 = Urgent Alert
        console_printf("[FEED PRODUCER] Emitting Market Tick #%u: '%s'\n", seq, payload);
        
        ipc_send_blocking(2, msg_type, payload);
        seq++;

        // Delay loop
        uint64_t start = timer_get_ticks();
        while (timer_get_ticks() - start < 10 && market_running) {
            asm volatile("pause");
        }
    }
}

static void order_matching_engine_task(void) {
    ipc_message_t msg;
    uint32_t order_id = 5000;

    while (market_running) {
        if (ipc_receive_blocking(&msg)) {
            if (msg.type == 999) {
                // Forward urgent volatility tick to Alert Engine immediately (PID priority boosting)
                console_printf("[MATCHING ENGINE] High Volatility Tick Detected! Forwarding to Alert Engine...\n");
                ipc_send_blocking(3, 999, msg.payload);
            } else {
                order_id++;
                console_printf("[MATCHING ENGINE] Executed Order #%u for Tick: %s\n", order_id, msg.payload);
                ipc_send_blocking(4, 200, "TRADE_EXECUTED_LOG");
            }
        }
    }
}

static void alert_engine_task(void) {
    ipc_message_t msg;

    while (market_running) {
        if (ipc_receive_blocking(&msg)) {
            if (msg.type == 999) {
                // Priority Boosted High Severity Alert Processing
                pcb_t *curr = process_get_current();
                uint32_t orig_priority = curr ? curr->priority : 9;
                if (curr) curr->priority = 10; // Boost to max priority during alert mitigation

                console_set_color(0x00FF0000, 0x00101018); // Bright Red Warning
                console_printf("\n[ALERT ENGINE - SEVERITY P0] HIGH MARKET VOLATILITY TRIGGERED: %s\n\n", msg.payload);
                console_set_color(0x00FFFFFF, 0x00101018); // Reset White

                if (curr) curr->priority = orig_priority; // Restore priority
            }
        }
    }
}

static void logger_task(void) {
    ipc_message_t msg;
    uint32_t log_count = 0;

    while (market_running) {
        if (ipc_receive_blocking(&msg)) {
            log_count++;
            console_printf("  [LOGGER PROCESS] Recorded Audit Log #%u: %s\n", log_count, msg.payload);
        }
    }
}

void market_init(void) {
    market_running = false;
}

void market_start_workload(void) {
    if (market_running) {
        console_write("[MARKET] Workload is already running!\n");
        return;
    }

    market_running = true;
    console_write("\n=====================================================\n");
    console_write("  Starting Preemptive IPC Market Workload Pipeline   \n");
    console_write("=====================================================\n");

    producer_pcb = process_create("Market_Producer", market_feed_producer_task, 6);
    matching_pcb = process_create("Matching_Engine", order_matching_engine_task, 5);
    alert_pcb    = process_create("Alert_Engine",    alert_engine_task,          9); // High priority
    logger_pcb   = process_create("Market_Logger",   logger_task,                 3);

    if (producer_pcb && matching_pcb && alert_pcb && logger_pcb) {
        console_printf("[MARKET] Spawned Producer (PID %u), Matching (PID %u), Alert (PID %u), Logger (PID %u)\n\n",
            producer_pcb->pid, matching_pcb->pid, alert_pcb->pid, logger_pcb->pid);
    } else {
        console_write("[ERR] Failed to spawn all market workload processes!\n\n");
    }
}

void market_stop_workload(void) {
    if (!market_running) {
        console_write("[MARKET] Workload is not running.\n");
        return;
    }

    market_running = false;
    if (producer_pcb) producer_pcb->state = PROCESS_TERMINATED;
    if (matching_pcb) matching_pcb->state = PROCESS_TERMINATED;
    if (alert_pcb)    alert_pcb->state    = PROCESS_TERMINATED;
    if (logger_pcb)   logger_pcb->state   = PROCESS_TERMINATED;

    console_write("\n[MARKET] Market workload pipeline stopped.\n\n");
}
