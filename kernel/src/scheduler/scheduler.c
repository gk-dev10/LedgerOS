#include "scheduler.h"
#include "../console/console.h"

#define SCHED_QUANTUM 5 // 5 ticks (50 ms at 100Hz)

static bool scheduler_active = false;
static volatile bool reschedule_pending = false;
static uint32_t active_task_index = 0;

void scheduler_init(void) {
    process_init();
    scheduler_active = true;
    reschedule_pending = false;
}

static pcb_t *scheduler_pick_next(void) {
    pcb_t *process_list = NULL;
    size_t count = process_get_all(&process_list);
    if (!process_list || count == 0) return NULL;

    // Multi-level Queue Priority Scheduling
    uint32_t max_priority = 0;
    int selected_idx = -1;

    // 1. Find highest priority ready process (excluding Idle process PID 0 if higher priority exists)
    for (size_t i = 1; i < count; i++) {
        if (process_list[i].state == PROCESS_READY && process_list[i].priority > max_priority) {
            max_priority = process_list[i].priority;
        }
    }

    if (max_priority > 0) {
        // Round-robin selection among processes with max_priority
        for (size_t i = 1; i <= count; i++) {
            size_t idx = (active_task_index + i) % count;
            if (idx != 0 && process_list[idx].state == PROCESS_READY && process_list[idx].priority == max_priority) {
                selected_idx = (int)idx;
                break;
            }
        }
    }

    // 2. If no user process is ready, fallback to Idle Process (PID 0)
    if (selected_idx == -1) {
        if (process_list[0].state == PROCESS_READY || process_list[0].state == PROCESS_RUNNING) {
            selected_idx = 0;
        }
    }

    if (selected_idx != -1) {
        active_task_index = (uint32_t)selected_idx;
        return &process_list[selected_idx];
    }

    return NULL;
}

void scheduler_yield(void) {
    if (!scheduler_active) return;

    pcb_t *curr = process_get_current();
    pcb_t *next = scheduler_pick_next();

    if (!next || next == curr) return;

    if (curr && curr->state == PROCESS_RUNNING) {
        curr->state = PROCESS_READY;
    }

    next->state = PROCESS_RUNNING;
    next->time_slice = 0;

    pcb_t *prev = curr;
    process_set_current(next);

    cpu_switch_to(&prev->rsp, next->rsp);
}

void scheduler_tick(void) {
    if (!scheduler_active) return;

    pcb_t *curr = process_get_current();
    if (curr) {
        curr->cpu_ticks++;
        curr->time_slice++;

        // Preempt task if quantum expired
        if (curr->time_slice >= SCHED_QUANTUM) {
            curr->time_slice = 0;
            // Do not context-switch inside the interrupt handler. Defer the
            // switch until scheduler_poll() runs in normal kernel context.
            reschedule_pending = true;
        }
    }
}

void scheduler_poll(void) {
    if (scheduler_active && reschedule_pending) {
        reschedule_pending = false;
        scheduler_yield();
    }
}

static void worker_task_alpha(void) {
    for (int i = 1; i <= 3; i++) {
        console_printf("[WORKER ALPHA] Processing trading signal batch #%d...\n", i);
        for (volatile uint32_t work = 0; work < 1000000; work++) {
            asm volatile("pause");
        }
        scheduler_yield();
    }
    console_write("[WORKER ALPHA] Completed.\n");
}

static void worker_task_beta(void) {
    for (int i = 1; i <= 3; i++) {
        console_printf("[WORKER BETA] Validating order book depth #%d...\n", i);
        for (volatile uint32_t work = 0; work < 1000000; work++) {
            asm volatile("pause");
        }
        scheduler_yield();
    }
    console_write("[WORKER BETA] Completed.\n");
}

void scheduler_start_demo(void) {
    console_write("\n--- Starting LedgerOS Cooperative Multi-Tasking Scheduler Demo ---\n");
    
    pcb_t *p1 = process_create("Worker_Alpha", worker_task_alpha, 5);
    pcb_t *p2 = process_create("Worker_Beta", worker_task_beta, 5);

    if (p1 && p2) {
        console_printf("[SCHED] Spawned Task 1 (PID %u: %s | Priority %u)\n", p1->pid, p1->name, p1->priority);
        console_printf("[SCHED] Spawned Task 2 (PID %u: %s | Priority %u)\n", p2->pid, p2->name, p2->priority);
        console_write("[SCHED] Dispatching workers; they yield between batches.\n\n");
        scheduler_yield();
        console_write("[SCHED] Worker demo completed.\n\n");
    } else {
        console_write("[SCHED] Failed to create demo processes!\n\n");
    }
}
