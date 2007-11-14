#include <l4/types.h>

void putc(int c)

{
  register long result asm("$0");
  register long type asm("$16") = 0;
  register long arg asm("$17") = (long) c;

  asm __volatile__ ("call_pal  %3" : "=r" (result) : "r" (type), "r" (arg), "i" (170));
}
