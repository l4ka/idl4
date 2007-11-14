	.text
	.global _start
	.align	16
	.proc	_start
_start:
	movl	gp = __gp
	movl	r8 = reg_stack
	movl	sp = stack
	mov	ar.rsc = 0		// turn off RSE
	;;
	mov	ar.bspstore = r8
	;;
	mov	ar.rsc = 3		// turn RSE back on
	;;
	alloc	r8 = ar.pfs,0,0,0,0
	br.sptk.many main

	/* NOTREACHED */
	.endp	_start

	.global _exreg_target
	.align	16
	.proc	_exreg_target
_exreg_target:
	/* load GP and RSE spots */
	movl	gp = __gp

	/* Load the jump address and reg-stack address from the stack */
	ld8     r9 = [sp], -8
		;; 
	ld8	r8 = [sp], -8
		;; 
	mov	b0 = r9
		;; 

	/* setup the RSE */
	mov	ar.rsc = 0
	;;
	mov	ar.bspstore = r8
	;;
	mov	ar.rsc = 3 + (3 << 2)
	;;
	alloc	r8 = ar.pfs,0,0,0,0
	br.sptk.many b0
	.endp	_exreg_target

	.bss
reg_stack:
	.space	8192
stack:
