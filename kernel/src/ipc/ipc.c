#include "ipc.h"
#include "../sync/spinlock.h"
#include "../memory.h"
#include "../console/console.h"
#include "../process/process.h"
#include "../timer/timer.h"

static ipc_message_t ipc_queue[IPC_QUEUE_CAPACITY];
static uint32_t ipc_head = 0;
static uint32_t ipc_tail = 0;
static uint32_t ipc_count = 0;
static spinlock_t ipc_lock;

// Simple Pseudo-Random Number Generator
static uint32_t prng_seed = 123456789;

static uint32_t prng_next(void) {
    prng_seed = prng_seed * 1103515245 + 12345 + (uint32_t)timer_get_ticks();
    return (prng_seed / 65536) % 32768;
}

void ipc_init(void) {
    spinlock_init(&ipc_lock);
    ipc_head = 0;
    ipc_tail = 0;
    ipc_count = 0;
    memset(ipc_queue, 0, sizeof(ipc_queue));
    prng_seed = (uint32_t)(timer_get_ticks() + 987654);
}

bool ipc_send(uint32_t receiver_pid, uint32_t type, const char *payload) {
    spinlock_lock(&ipc_lock);

    if (ipc_count >= IPC_QUEUE_CAPACITY) {
        spinlock_unlock(&ipc_lock);
        return false;
    }

    pcb_t *curr = process_get_current();
    ipc_message_t *msg = &ipc_queue[ipc_head];
    msg->sender_pid = curr ? curr->pid : 0;
    msg->receiver_pid = receiver_pid;
    msg->type = type;

    memset(msg->payload, 0, IPC_PAYLOAD_SIZE);
    if (payload) {
        size_t len = 0;
        while (payload[len] && len < IPC_PAYLOAD_SIZE - 1) len++;
        memcpy(msg->payload, payload, len);
    }

    ipc_head = (ipc_head + 1) % IPC_QUEUE_CAPACITY;
    ipc_count++;

    spinlock_unlock(&ipc_lock);
    return true;
}

bool ipc_receive(ipc_message_t *msg) {
    if (!msg) return false;

    spinlock_lock(&ipc_lock);

    if (ipc_count == 0) {
        spinlock_unlock(&ipc_lock);
        return false;
    }

    memcpy(msg, &ipc_queue[ipc_tail], sizeof(ipc_message_t));
    ipc_tail = (ipc_tail + 1) % IPC_QUEUE_CAPACITY;
    ipc_count--;

    spinlock_unlock(&ipc_lock);
    return true;
}

void ipc_start_demo(void) {
    console_write("\n--- Starting Dynamic Live Market Feed & IPC Demo ---\n");

    const char *tickers[] = {"AAPL", "NVDA", "TSLA", "BTC", "ETH", "GOOGL"};
    
    for (int i = 0; i < 3; i++) {
        uint32_t r = prng_next();
        const char *t = tickers[r % 6];
        uint32_t price = 100 + (r % 500);
        uint32_t cents = (r * 17) % 99;

        char buf[64];
        // Format string manually
        buf[0] = t[0]; buf[1] = t[1]; buf[2] = t[2]; buf[3] = t[3] ? t[3] : '\0';
        size_t pos = t[3] ? 4 : 3;
        buf[pos++] = ':'; buf[pos++] = ' '; buf[pos++] = '$';
        
        // append price
        char pnum[16];
        int pi = 0;
        uint32_t tmp = price;
        if (tmp == 0) pnum[pi++] = '0';
        while (tmp > 0) { pnum[pi++] = '0' + (tmp % 10); tmp /= 10; }
        while (pi > 0) buf[pos++] = pnum[--pi];

        buf[pos++] = '.';
        buf[pos++] = '0' + (cents / 10);
        buf[pos++] = '0' + (cents % 10);
        buf[pos] = '\0';

        console_printf("[PRODUCER] Generated Live Tick -> IPC Mailbox: '%s'\n", buf);
        ipc_send(2, 100 + i, buf);
    }

    console_write("\n[CONSUMER Alert Process] Draining IPC Mailbox via Spinlocks:\n");

    ipc_message_t msg;
    int count = 0;
    while (ipc_receive(&msg)) {
        count++;
        console_printf("  <- MSG #%d [Sender PID %u -> Receiver PID %u | Type %u]: %s\n",
            count, msg.sender_pid, msg.receiver_pid, msg.type, msg.payload);
    }

    console_write("\n[IPC] Dynamic message passing demonstration finished.\n\n");
}
