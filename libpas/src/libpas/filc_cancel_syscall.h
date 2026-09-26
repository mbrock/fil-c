/* Native Linux cancellation gate. The signal handler may redirect execution
   from [filc_cancel_syscall_begin, filc_cancel_syscall_end) to
   filc_cancel_syscall_abort. All paths return normally to the native caller.
   The gate never invokes Fil-C or unwinds a native signal frame. */
#ifndef FILC_CANCEL_SYSCALL_H
#define FILC_CANCEL_SYSCALL_H

#ifndef __ASSEMBLER__
typedef struct {
    long value;
    long canceled;
} filc_cancel_syscall_result;

/* State bits match the glibc cancellation control word. Other bits are ignored.
   This is an internal native interface, not an unchecked Fil-C syscall API. */
#define FILC_CANCEL_DISABLED 1u
#define FILC_CANCEL_ASYNC 2u
#define FILC_CANCEL_PENDING 8u
#define FILC_CANCEL_EXITING 16u

extern filc_cancel_syscall_result filc_cancel_syscall(
    const unsigned* state, long number,
    long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
extern const char filc_cancel_syscall_begin[], filc_cancel_syscall_end[];
extern const char filc_cancel_syscall_abort[];
#endif

#endif
