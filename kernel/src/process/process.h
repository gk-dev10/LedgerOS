#ifndef LEDGEROS_PROCESS_H
#define LEDGEROS_PROCESS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_PROCESSES 16
#define KERNEL_STACK_SIZE (16 * 1024) // 16 KB Stack per process

typedef enum {
    PROCESS_UNUSED = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

typedef struct pcb {
    uint32_t pid;
    char name[32];
    process_state_t state;
    uint32_t priority;
    uint64_t cpu_ticks;
    uint64_t rsp;
    void *stack;
} pcb_t;

void process_init(void);
pcb_t *process_create(const char *name, void (*entry_point)(void), uint32_t priority);
pcb_t *process_get_current(void);
pcb_t *process_get_by_id(uint32_t pid);
size_t process_get_all(pcb_t **process_list);

#endif
