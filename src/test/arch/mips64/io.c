#include <l4/types.h>

#define L4_TRAP_KPUTC 0

void putc(int c)

{
  register long r_c asm("$4") = c;

  __asm__ __volatile__ (
      ".set noat\n\t"
      "li     $1, %0\n\t"
      "break\n\t"
      ".set at\n\t"
      : : "i" (L4_TRAP_KPUTC), "r" (r_c) : "$1"
  );
}
