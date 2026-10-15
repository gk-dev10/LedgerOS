.global cpu_switch_to
.type cpu_switch_to, @function

/*
 * cpu_switch_to(uint64_t *old_rsp, uint64_t new_rsp)
 * RDI = pointer to current process PCB's rsp field (uint64_t *)
 * RSI = target process's rsp (uint64_t)
 */
cpu_switch_to:
    /* Save flags and callee-saved registers of current process */
    pushfq
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    /* Save current stack pointer into *old_rsp if old_rsp is non-null */
    testq %rdi, %rdi
    jz 1f
    movq %rsp, (%rdi)

1:
    /* Switch stack pointer to new_rsp */
    movq %rsi, %rsp

    /* Restore callee-saved registers of next process */
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    popfq

    ret

.section .note.GNU-stack,"",@progbits
