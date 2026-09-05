#ifndef LEDGEROS_IPC_H
#define LEDGEROS_IPC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define IPC_PAYLOAD_SIZE 64
#define IPC_QUEUE_CAPACITY 16

typedef struct {
    uint32_t sender_pid;
    uint32_t receiver_pid;
    uint32_t type;
    char payload[IPC_PAYLOAD_SIZE];
} ipc_message_t;

void ipc_init(void);
bool ipc_send(uint32_t receiver_pid, uint32_t type, const char *payload);
bool ipc_receive(ipc_message_t *msg);
void ipc_start_demo(void);

#endif
