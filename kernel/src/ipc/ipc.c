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

void ipc_init(void) {
    spinlock_init(&ipc_lock);
    ipc_head = 0;
    ipc_tail = 0;
    ipc_count = 0;
    memset(ipc_queue, 0, sizeof(ipc_queue));
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
    console_write("\n--- Starting LedgerOS IPC & Synchronization Demo ---\n");
    
    console_write("[PRODUCER Feed] Pushing price tick: 'AAPL @ $189.45' into IPC Mailbox...\n");
    ipc_send(2, 100, "AAPL: $189.45 (BUY SIGNAL)");
    
    console_write("[PRODUCER Feed] Pushing price tick: 'NVDA @ $512.80' into IPC Mailbox...\n");
    ipc_send(2, 101, "NVDA: $512.80 (ALERT HIGH)");

    console_write("[PRODUCER Feed] Pushing price tick: 'TSLA @ $238.10' into IPC Mailbox...\n\n");
    ipc_send(2, 102, "TSLA: $238.10 (NORMAL)");

    console_write("[CONSUMER Alert] Reading messages from IPC Mailbox:\n");

    ipc_message_t msg;
    int msg_count = 0;
    while (ipc_receive(&msg)) {
        msg_count++;
        console_printf("  <- MSG #%d [From PID %u -> PID %u | Type %u]: %s\n",
            msg_count, msg.sender_pid, msg.receiver_pid, msg.type, msg.payload);
    }

    console_write("\n[IPC] Synchronization and ring-buffer transmission complete.\n\n");
}
