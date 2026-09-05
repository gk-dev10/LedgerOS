#include "process.h"
#include "../memory/heap.h"
#include "../memory.h"
#include "../console/console.h"

static pcb_t process_table[MAX_PROCESSES];
static uint32_t current_pid = 0;
static uint32_t next_pid = 1;

void process_init(void) {
    memset(process_table, 0, sizeof(process_table));

    // Initialize Main Kernel Process (PID 0)
    pcb_t *main_pcb = &process_table[0];
    main_pcb->pid = 0;
    memcpy(main_pcb->name, "Kernel Main", 12);
    main_pcb->state = PROCESS_RUNNING;
    main_pcb->priority = 10;
    main_pcb->cpu_ticks = 0;
    main_pcb->stack = NULL;
    main_pcb->rsp = 0;

    current_pid = 0;
}

pcb_t *process_get_current(void) {
    return &process_table[current_pid];
}

pcb_t *process_get_by_id(uint32_t pid) {
    for (size_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROCESS_UNUSED && process_table[i].pid == pid) {
            return &process_table[i];
        }
    }
    return NULL;
}

size_t process_get_all(pcb_t **process_list) {
    if (process_list) {
        *process_list = process_table;
    }
    return MAX_PROCESSES;
}

static void process_wrapper(void (*entry)(void)) {
    if (entry) {
        entry();
    }
    pcb_t *curr = process_get_current();
    curr->state = PROCESS_TERMINATED;
    for (;;) {
        asm volatile ("hlt");
    }
}

pcb_t *process_create(const char *name, void (*entry_point)(void), uint32_t priority) {
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_UNUSED || process_table[i].state == PROCESS_TERMINATED) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        console_write("[ERR] Process Table Full!\n");
        return NULL;
    }

    pcb_t *pcb = &process_table[slot];
    memset(pcb, 0, sizeof(pcb_t));
    pcb->pid = next_pid++;
    
    size_t name_len = 0;
    while (name[name_len] && name_len < 31) name_len++;
    memcpy(pcb->name, name, name_len);
    pcb->name[name_len] = '\0';

    pcb->priority = priority;
    pcb->state = PROCESS_READY;
    pcb->cpu_ticks = 0;
    pcb->stack = kmalloc(KERNEL_STACK_SIZE);

    if (!pcb->stack) {
        pcb->state = PROCESS_UNUSED;
        return NULL;
    }

    // Set up stack for context switching
    uint64_t *stack_top = (uint64_t *)((uint8_t *)pcb->stack + KERNEL_STACK_SIZE);
    
    // Push entry point wrapper and registers
    *(--stack_top) = (uint64_t)entry_point; // Return address for wrapper
    *(--stack_top) = (uint64_t)process_wrapper; // RIP
    *(--stack_top) = 0x202; // RFLAGS (Interrupts Enabled)
    
    // Save dummy registers (rax, rbx, rcx, rdx, rsi, rdi, rbp, r8..r15)
    for (int r = 0; r < 14; r++) {
        *(--stack_top) = 0;
    }

    pcb->rsp = (uint64_t)stack_top;
    return pcb;
}
