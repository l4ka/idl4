#ifndef __chacmos_stdlib_h
#define __chacmos_stdlib_h

#define L4_INV 0xFFFFFFFF

extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, unsigned max);
extern int strcmp(const char *a, const char *b);
extern unsigned int strlen(const char *s);
extern int printf(const char *fmt, ...);
extern void panic(const char *fmt, ...) __attribute__ ((noreturn));
extern int sprintf(char *dest, const char *fmt, ...);
extern char *strchr(const char *s, int c);
extern char *strtok(char *s, const char *delim);
extern char *strcat(char *dest, const char *src);
extern unsigned char getc();

#define CORBA_free(a) free(a)
#define CORBA_string_alloc(a) ((char*)malloc(a))
void *malloc(unsigned int size);
void free(void *what);

#endif
