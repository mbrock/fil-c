/* The final request check and syscall instruction must share one PC window.
   In particular, the instruction immediately after syscall is outside it:
   Linux resumes there when returning a completed/partial result, and at the
   syscall instruction when restarting an interrupted operation. */
#if defined(__x86_64__)
    .text
    .p2align 4
    .globl filc_cancel_syscall
    .hidden filc_cancel_syscall
    .type filc_cancel_syscall,@function
filc_cancel_syscall:
    .cfi_startproc
    .globl filc_cancel_syscall_begin
    .hidden filc_cancel_syscall_begin
filc_cancel_syscall_begin:
    movl (%rdi), %eax
    andl $25, %eax                /* pending, enabled, not exiting */
    cmpl $8, %eax
    je filc_cancel_syscall_abort
    movq %rsi, %rax               /* syscall number */
    movq %rdx, %rdi               /* argument 1 */
    movq %rcx, %rsi               /* argument 2 */
    movq %r8, %rdx                /* argument 3 */
    movq %r9, %r10                /* argument 4 */
    movq 8(%rsp), %r8             /* argument 5 */
    movq 16(%rsp), %r9            /* argument 6 */
    syscall
    .globl filc_cancel_syscall_end
    .hidden filc_cancel_syscall_end
filc_cancel_syscall_end:
    xorl %edx, %edx               /* result.canceled = false */
    ret
    .globl filc_cancel_syscall_abort
    .hidden filc_cancel_syscall_abort
filc_cancel_syscall_abort:
    xorl %eax, %eax               /* no kernel result */
    movl $1, %edx                 /* result.canceled = true */
    ret
    .cfi_endproc
    .size filc_cancel_syscall, .-filc_cancel_syscall
    .section .note.GNU-stack,"",@progbits
#endif
