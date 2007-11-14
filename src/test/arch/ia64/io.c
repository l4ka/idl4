#include <l4/types.h>

static void ia64_putc (int c)
{
    register char r14 asm ("r14") = c;

    asm volatile (
	"{ .mlx					\n"
	"	break.m	0x1			\n"
	"	movl	r0 = 0 ;;		\n"
	"}					\n"
	:
	:
	"r" (r14));
}

void putc (int c)
{
    ia64_putc (c);
    if (c == '\n')
	ia64_putc ('\r');
}
