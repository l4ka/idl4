#ifndef __idl4_test_generic_h__
#define __idl4_test_generic_h__

#if defined(IDL4_INTERNAL) && defined(CONFIG_ARCH_IA32)
#define IDL4_OMIT_FRAME_POINTER 0
#endif

#define NULL ((void*)0)

#include <idl4/idl4.h>

#define CMD_RUN			0x0000
#define CMD_DELAY		0x9999
#define STATUS_OK 		0x0ABC
#define STATUS_DELAYED		0x0BCD
#define STATUS_EXCEPTION	0x1000

#define IDL4_STACK_MAGIC 0xC01AD05Eu

#ifdef __cplusplus
extern "C" 

{
#endif
  int printf(const char *fmt, ...);
  char *strcpy(char *dest, const char *src);
  int strcmp(const char *a, const char *b);
  char *strncpy(char *dest, const char *src, idl4_size_t max);
  char *strchr(const char *s, int c);
  char *strtok(char *s, const char *delim);
  char *strcat(char *dest, const char *src);
  idl4_size_t strlen(const char *s);
  int sprintf(char *dest, const char *fmt, ...);
  void panic(const char *fmt, ...);
  unsigned int random(void);
  void *randomPtr(void);
  void *malloc(unsigned int size);
  void *smalloc(unsigned int size);
  void *firstalloc(void);
  void free(void *ptr);
  void sfree(void);
  void putc(int c);
#ifdef __cplusplus
}  
#endif

#include IDL4_INC_API(test.h)

#endif /* !defined(__idl4_test_generic_h__) */
