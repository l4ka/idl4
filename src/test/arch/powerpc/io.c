#include <l4/types.h>
#include <l4/powerpc/kdebug.h>

void putc(int c)

{
  L4_KDB_PrintChar(c);
  if (c == '\n')
    L4_KDB_PrintChar('\r');
}
