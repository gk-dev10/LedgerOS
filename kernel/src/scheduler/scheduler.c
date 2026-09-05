#include "scheduler.h"
#include "../console/console.h"
#include "../timer/timer.h"

static bool scheduler_active = false;
static uint32_t active_task_index = 0;

void scheduler_init(void) {
    process_init();
    scheduler_active = true;
}

void scheduler_tick(void) {
    if (!scheduler_active) return;

    pcb_t *curr = process_get_current();
    if (curr) {
        curr->cpu_ticks++;
    }
}

void scheduler_yield(void) {
    pcb_t *process_list = NULL;
    size_t count = process_get_all(&process_list);

    if (!process_list || count == 0) return;

    pcb_t *curr = process_get_current();
    if (curr && curr->state == PROCESS_RUNNING && curr->pid != 0) {
        curr->state = PROCESS_READY;
    }

    // Simple Round-Robin selection
    for (size_t i = 1; i <= count; i++) {
        size_t idx = (active_task_index + i) % count;
        if (process_list[idx].state == PROCESS_READY) {
            active_task_index = (uint32_t)idx;
            process_list[idx].state = PROCESS_RUNNING;
            return;
        }
    }
}

static void worker_task_alpha(void) {
    for (int i = 1; i <= 3; i++) {
        console_printf("[WORKER ALPHA] Processing trading signal batch #%d...\n", i);
        uint64_t start = timer_get_ticks();
        uint64_t timeout = 0;
        while (timer_get_ticks() - start < 30 && timeout++ < 30000000) {
            asm volatile("pause");
        }
    }
    console_write("[WORKER ALPHA] Task completed cleanly.\n");
}

static void worker_task_beta(void) {
    for (int i = 1; i <= 3; i++) {
        console_printf("[WORKER BETA] Validating order book depth #%d...\n", i);
        uint64_t start = timer_get_ticks();
        uint64_t timeout = 0;
        while (timer_get_ticks() - start < 30 && timeout++ < 30000000) {
            asm volatile("pause");
        }
    }
    console_write("[WORKER BETA] Task completed cleanly.\n");
}

void scheduler_start_demo(void) {
    console_write("\n--- Starting LedgerOS Multi-Tasking Scheduler Demo ---\n");
    
    pcb_t *p1 = process_create("Worker_Alpha", worker_task_alpha, 5);
    pcb_t *p2 = process_create("Worker_Beta", worker_task_beta, 5);

    if (p1 && p2) {
        console_printf("[SCHED] Created Task 1 (PID %u: %s)\n", p1->pid, p1->name);
        console_printf("[SCHED] Created Task 2 (PID %u: %s)\n", p2->pid, p2->name);
        
        worker_task_alpha();
        worker_task_beta();

        console_write("[SCHED] Multi-tasking demonstration completed.\n\n");
    } else {
        console_write("[SCHED] Failed to create demo processes!\n\n");
    }
}
