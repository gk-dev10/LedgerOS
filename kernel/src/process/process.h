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
    uint32_t priority;      // 0 (Idle) to 10 (Highest)
    uint64_t cpu_ticks;
    uint64_t rsp;
    void *stack;
    uint64_t time_slice;
    void (*entry_point)(void);
} pcb_t;

void process_init(void);
pcb_t *process_create(const char *name, void (*entry_point)(void), uint32_t priority);
pcb_t *process_get_current(void);
void process_set_current(pcb_t *proc);
pcb_t *process_get_by_id(uint32_t pid);
size_t process_get_all(pcb_t **process_list);

/* Assembly subroutine in switch.S */
extern void cpu_switch_to(uint64_t *old_rsp, uint64_t new_rsp);

#endif
