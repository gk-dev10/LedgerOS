#include "syscall.h"
#include "../console/console.h"
#include "../process/process.h"
#include "../scheduler/scheduler.h"
#include "../ipc/ipc.h"
#include "../timer/timer.h"
#include "../fs/ledgerfs.h"

#define MSR_EFER     0xC0000080
#define MSR_STAR     0xC0000081
#define MSR_LSTAR    0xC0000082
#define MSR_SFMASK   0xC0000084

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    asm volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    uint32_t low = (uint32_t)val;
    uint32_t high = (uint32_t)(val >> 32);
    asm volatile ("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

void syscall_init(void) {
    // Enable SCE (System Call Extensions) bit 0 in EFER MSR
    uint64_t efer = rdmsr(MSR_EFER);
    wrmsr(MSR_EFER, efer | 1ULL);

    // Set STAR MSR: CS/SS selectors for SYSRET/SYSCALL
    wrmsr(MSR_STAR, ((uint64_t)0x1B << 48) | ((uint64_t)0x08 << 32));

    // Set LSTAR MSR to syscall_entry handler address
    wrmsr(MSR_LSTAR, (uint64_t)(uintptr_t)syscall_entry);

    // Set SFMASK MSR (mask RFLAGS interrupt flag during syscall entry if desired)
    wrmsr(MSR_SFMASK, 0x200);

    console_printf("[SYSCALL] Initialized x86-64 SYSCALL/SYSRET MSR Dispatcher (LSTAR: 0x%x)\n", (uint32_t)(uintptr_t)syscall_entry);
}

uint64_t syscall_handler(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    (void)arg3;

    switch (sys_num) {
        case SYS_SPAWN: {
            pcb_t *p = process_create((const char *)arg1, (void (*)(void))arg2, (uint32_t)arg3);
            return p ? p->pid : 0;
        }
        case SYS_YIELD: {
            scheduler_yield();
            return 0;
        }
        case SYS_IPC_SEND: {
            return ipc_send_blocking((uint32_t)arg1, (uint32_t)arg2, (const char *)arg3) ? 1 : 0;
        }
        case SYS_IPC_RECV: {
            return ipc_receive_blocking((ipc_message_t *)arg1) ? 1 : 0;
        }
        case SYS_GETTIME: {
            return timer_get_ticks();
        }
        case SYS_LOG: {
            return ledgerfs_write_log((const char *)arg1) ? 1 : 0;
        }
        default:
            console_printf("[SYSCALL] Unknown system call number: %u\n", (uint32_t)sys_num);
            return (uint64_t)-1;
    }
}
