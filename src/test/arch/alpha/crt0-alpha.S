        .global _start
        .align  3
        .ent    _start

_start:	bsr	$27, 1f
1:	ldgp	$29, 0($27)
		
	lda	$30, init_stack
	lda	$1, 8192
	addq	$30, $1, $30

	lda	$27, __L4_Init
	jsr	$26, ($27)

	ldgp	$29, 0($26)
	
	lda	$27, main
	jsr	$26, ($27), main

	lda	$16, 2
	call_pal 170
        
        .end    _start

        .global _exreg_target
        .align  3
        .ent    _exreg_target   
        
_exreg_target:

	/* get our GP */
	bsr	$27, 1f
1:	ldgp	$29, 0($27)
	
	ldq	$27, 0($30)       /* load IP off the stack */
	jmp	$26, ($27), 0x1   /* jump to the IP */
	
2:	lda	$16, 2
	call_pal 170
        
        .end    _exreg_target

.lcomm init_stack, 8192
