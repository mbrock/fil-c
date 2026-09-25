# gas accepts several `;`-separated statements per line; preprocessed .S
# macros produce such lines. Each statement must be checked on its own, and
# `#` comments -- including ones containing `;`, `"`, or `#!` text -- must
# neither split a line nor attach an annotation.
	.text
	.p2align 4
	.globl	semi_sum
	.type	semi_sum, @function
semi_sum:                       ;! long(ptr, long)
	xorl %eax, %eax; testq %rsi, %rsi; jz 2f   # empty; skip the "loop
1:	addq (%rdi), %rax; addq $8, %rdi; decq %rsi; jnz 1b # a; b #! load ptr
2:	ret; # trailing comment after an empty statement
	.size	semi_sum, .-semi_sum

	.globl	semi_inc
	.type	semi_inc, @function
semi_inc:                       ;! long(ptr)
	lock; incq (%rdi); movq (%rdi), %rax; ret
	.size	semi_inc, .-semi_inc

	.globl	semi_load_ptr
	.type	semi_load_ptr, @function
semi_load_ptr:                  ;! ptr(ptr)
	nop; movq (%rdi), %rax #! load ptr
	ret
	.size	semi_load_ptr, .-semi_load_ptr

	.section .rodata
	.globl	semi_str
	.type	semi_str, @object
semi_str: .asciz "a;b#c\";d"; .byte 0
	.size	semi_str, .-semi_str
	.section	.note.GNU-stack,"",@progbits
