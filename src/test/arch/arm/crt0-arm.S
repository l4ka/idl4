	.globl	_start
_start:
	ldr	sp, 2f
	bl	main
1:	b	1b
2:	.word	_init_stack_top

	.bss
	.p2align 12
_init_stack_bottom:
	.space	8192
_init_stack_top:
