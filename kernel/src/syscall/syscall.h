#ifndef LEDGEROS_SYSCALL_H
#define LEDGEROS_SYSCALL_H

#include <stdint.h>
#include <stdbool.h>

#define SYS_SPAWN    1
#define SYS_YIELD    2
#define SYS_IPC_SEND 3
#define SYS_IPC_RECV 4
#define SYS_GETTIME  5
#define SYS_LOG      6

void syscall_init(void);
uint64_t syscall_handler(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3);

/* Assembly SYSCALL vector handler */
extern void syscall_entry(void);

#endif
