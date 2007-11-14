void take(volatile int *spinlock)

{
  __asm__ __volatile__ (
	"1:	mov	r30 = 1					\n"
	"	mov	ar.ccv = r0				\n"
	"	;;						\n"
	"	cmpxchg8.acq r30 = [%0], r30, ar.ccv		\n"
	"	;;						\n"
	"	cmp.ne	p15,p0 = r30, r0			\n"
	"	;;						\n"
	"(p15)	br.spnt.few 1b					\n"
	:
	: "r" (spinlock)
	: "ar.ccv", "p15", "b7", "r30", "memory"
  );
}

void release(volatile int *spinlock)

{
  __asm__ __volatile__ (
	"	st8.rel	[%0] = r0				\n"
	"	;;						\n"
	:
	: "r" (spinlock)
  );
}
