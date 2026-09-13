.global syscall_entry
.type syscall_entry, @function

/*
 * Assembly handler for x86-64 SYSCALL instruction.
 * Registers on entry:
 * RDI = sys_num
 * RSI = arg1
 * RDX = arg2
 * RCX = arg3 (or return RIP saved by CPU)
 * R10 = actual arg3 passed by user wrapper
 * R11 = RFLAGS saved by CPU
 */
syscall_entry:
    /* Push saved RIP and RFLAGS */
    pushq %rcx
    pushq %r11

    /* Save caller registers */
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    /* Dispatch to C handler: syscall_handler(rdi, rsi, rdx, r10) */
    movq %r10, %rcx
    call syscall_handler

    /* Restore caller registers */
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx

    /* Restore RFLAGS and RIP */
    popq %r11
    popq %rcx

    sysretq

.section .note.GNU-stack,"",@progbits
