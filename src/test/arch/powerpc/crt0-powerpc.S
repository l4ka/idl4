	.section ".data"
	.align	2
	.globl	_l4_utcb
_l4_utcb:
	.long	0

	.section ".text"
	.align	2
	.globl	_start
_start:
	/* Install our stack. */
	lis	%r1, _stack_top@ha
	la	%r1, _stack_top@l(%r1)
	addi	%r1, %r1, -32

	/* Point to 0 for the small data area. */
	li	%r13, 0

	/* Save the utcb pointer handed us by the kernel in r2. */
	lis	%r10, _l4_utcb@ha
	la	%r10, _l4_utcb@l(%r10)
	stw	%r2, 0(%r10)

	bl	__L4_Init

	/* Jump into C code. */
	bl	main

1:	b	1b	/* Spin. */

/****************************************************************************/

	.section ".bss"
	.globl _stack_bottom
	.globl _stack_top

_stack_bottom:
.lcomm	_stack, 4096*3, 16
_stack_top:

