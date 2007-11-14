/*
 * arch/alpha/lib/divide.S
 *
 * (C) 1995 Linus Torvalds
 *
 * Alpha division..
 */

/*
 * The alpha chip doesn't provide hardware division, so we have to do it
 * by hand.  The compiler expects the functions
 *
 *	__divqu: 64-bit unsigned long divide
 *	__remqu: 64-bit unsigned long remainder
 *	__divqs/__remqs: signed 64-bit
 *	__divlu/__remlu: unsigned 32-bit
 *	__divls/__remls: signed 32-bit
 *
 * These are not normal C functions: instead of the normal
 * calling sequence, these expect their arguments in registers
 * $24 and $25, and return the result in $27. Register $28 may
 * be clobbered (assembly temporary), anything else must be saved. 
 *
 * In short: painful.
 *
 * This is a rather simple bit-at-a-time algorithm: it's very good
 * at dividing random 64-bit numbers, but the more usual case where
 * the divisor is small is handled better by the DEC algorithm
 * using lookup tables. This uses much less memory, though, and is
 * nicer on the cache.. Besides, I don't know the copyright status
 * of the DEC code.
 */

/*
 * My temporaries:
 *	$0 - current bit
 *	$1 - shifted divisor
 *	$2 - modulus/quotient
 *
 *	$23 - return address
 *	$24 - dividend
 *	$25 - divisor
 *
 *	$27 - quotient/modulus
 *	$28 - compare status
 */



/*
 * Select function type and registers
 */











/*
 * For 32-bit operations, we need to extend to 64-bit
 */







.set noat
.globl	__remqu   
.ent	__remqu   
__remqu   :
	subq	$30,32,$30
	stq	$0, 0($30)
	stq	$1, 8($30)
	stq	$2,16($30)

	bis	$25,$25,$1 
	bis	$24,$24,$27 
	bis	$31,$31,$2 
	 
	 
	beq	$1 , 9f			/* div by zero */
	bis	$31,1,$0 

	/* shift divisor left */
1:	cmpult	$1 ,$27 ,$28 
	blt	$1 , 3f
	addq	$1 ,$1 ,$1 
	addq	$0 ,$0 ,$0 
	bne	$28 ,1b

	/* ok, start to go right again.. */
2:	srl	$1 ,1,$1 
	beq	$0 ,9f
	srl	$0 ,1,$0 
3:	cmpule	$1 ,$27 ,$28 
	beq	$28 ,2b
	addq	$2 ,$0 ,$2 
	beq	$0 ,9f
	subq	$27 ,$1 ,$27 
	br	2b

9:	ldq	$0, 0($30)
	ldq	$1, 8($30)
	ldq	$2, 16($30)
	addq	$30,32,$30
	ret	$31,($23),1
	.end	__remqu   

/*
 * Uhh.. Ugly signed division. I'd rather not have it at all, but
 * it's needed in some circumstances. There are different ways to
 * handle this, really. This does:
 * 	-a / b = a / -b = -(a / b)
 *	-a % b = a % b
 *	a % -b = -(a % b)
 * which is probably not the best solution, but at least should
 * have the property that (x/y)*y + (x%y) = x.
 */
.globl	__remq   
.ent	__remq   
__remq   :
	bis	$24,$25,$28
	 
	bge	$28,__remqu   
	subq	$30,32,$30
	stq	$23,0($30)
	stq	$24,8($30)
	stq	$25,16($30)
	subq	$31,$24,$28
	cmovlt	$24,$28,$24	/* abs($24) */
	subq	$31,$25,$28
	cmovlt	$25,$28,$25	/* abs($25) */
	bsr	$23,__remqu   
	ldq	$23,0($30)
	ldq	$24,8($30)
	ldq	$25,16($30)
	addq	$30,32,$30
	bis $24,$24, $28  
	 
	bge	$28,1f
	subq	$31,$27,$27
1:	ret	$31,($23),1
	.end	__remq   
