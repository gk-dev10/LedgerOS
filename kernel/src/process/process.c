#include "process.h"
#include "../memory/heap.h"
#include "../memory.h"
#include "../console/console.h"
#include "../scheduler/scheduler.h"

static pcb_t process_table[MAX_PROCESSES];
static pcb_t *current_pcb = NULL;
static uint32_t next_pid = 1;

static void idle_process_entry(void) {
    for (;;) {
        asm volatile ("hlt");
    }
}

static void process_wrapper(void) {
    pcb_t *curr = process_get_current();
    if (curr && curr->entry_point) {
        curr->entry_point();
    }
    
    if (curr) {
        curr->state = PROCESS_TERMINATED;
        console_printf("[PROC] Process '%s' (PID %u) terminated.\n", curr->name, curr->pid);
    }
    
    scheduler_yield();
    for (;;) {
        asm volatile ("hlt");
    }
}

void process_init(void) {
    memset(process_table, 0, sizeof(process_table));

    // Initialize Idle Process (PID 0)
    pcb_t *idle_pcb = &process_table[0];
    idle_pcb->pid = 0;
    memcpy(idle_pcb->name, "Idle Process", 13);
    idle_pcb->state = PROCESS_READY;
    idle_pcb->priority = 0; // Lowest priority
    idle_pcb->cpu_ticks = 0;
    idle_pcb->entry_point = idle_process_entry;
    idle_pcb->stack = kmalloc(KERNEL_STACK_SIZE);

    if (idle_pcb->stack) {
        uint64_t *stack_top = (uint64_t *)((uint8_t *)idle_pcb->stack + KERNEL_STACK_SIZE);
        *(--stack_top) = (uint64_t)process_wrapper; // Return address
        *(--stack_top) = 0x202; // Initial RFLAGS: reserved bit 1 and interrupts enabled
        for (int i = 0; i < 6; i++) {
            *(--stack_top) = 0; // r15, r14, r13, r12, rbp, rbx
        }
        idle_pcb->rsp = (uint64_t)stack_top;
    }

    current_pcb = &process_table[0];
    current_pcb->state = PROCESS_RUNNING;
}

pcb_t *process_get_current(void) {
    return current_pcb;
}

void process_set_current(pcb_t *proc) {
    if (proc) {
        current_pcb = proc;
    }
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

bool process_set_priority(uint32_t pid, uint32_t priority) {
    pcb_t *p = process_get_by_id(pid);
    if (!p) return false;
    if (priority > 10) priority = 10;
    p->priority = priority;
    return true;
}

pcb_t *process_create(const char *name, void (*entry_point)(void), uint32_t priority) {
    int slot = -1;
    for (int i = 1; i < MAX_PROCESSES; i++) {
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
    while (name && name[name_len] && name_len < 31) name_len++;
    if (name) memcpy(pcb->name, name, name_len);
    pcb->name[name_len] = '\0';

    pcb->priority = priority;
    pcb->state = PROCESS_READY;
    pcb->cpu_ticks = 0;
    pcb->entry_point = entry_point;
    pcb->stack = kmalloc(KERNEL_STACK_SIZE);

    if (!pcb->stack) {
        pcb->state = PROCESS_UNUSED;
        return NULL;
    }

    // Set up stack frame for cpu_switch_to
    uint64_t *stack_top = (uint64_t *)((uint8_t *)pcb->stack + KERNEL_STACK_SIZE);
    
    // Return address when cpu_switch_to executes 'ret'
    *(--stack_top) = (uint64_t)process_wrapper;
    *(--stack_top) = 0x202; // Initial RFLAGS: reserved bit 1 and interrupts enabled
    
    // 6 Callee-saved registers (r15, r14, r13, r12, rbp, rbx)
    for (int r = 0; r < 6; r++) {
        *(--stack_top) = 0;
    }

    pcb->rsp = (uint64_t)stack_top;
    return pcb;
}
