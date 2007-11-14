#include <idl4/test.h>

void *memcpy(void *dest, const void *source, idl4_size_t n)

{
  char *dst = dest;
  const char *src = source;

  while (n--) 
    *dst++ = *src++;

  return dest;
}

void *memset(void *s, char c, idl4_size_t n)

{
  char *dst = s;

  while (n--) 
    *dst++ = c;

  return s;
}
