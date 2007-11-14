#include <l4/l4.h>
#include <stdlib.h>

char * tokpos = NULL;

// strcpy

char *strcpy(char *dest, const char *src)

{	
  while (*src) 
    *dest++=*src++;
  *dest=0;  
  return dest;  
}

int strcmp(const char *a, const char *b)

{
  while ((*a) && (*a==*b))
    { a++;b++; }  
  if (*a==*b) return 0;  
  if (*a<*b) return -1;
  return 1;  
}


char *strncpy(char *dest, const char *src, unsigned max)

{	
  char *cdest = dest;
  while ((*src) && (max>0))
    { *cdest++=*src++;max--; }
  *cdest=0;  
  return dest;
}

char *strchr(const char *s, int c)

{
  while (*s!=c)
    {
      if (!*s) return NULL;
      s++;
    }  
    
  return (char*)s;
}    

char *strtok(char *s, const char *delim)

{
  char *begin = (s==NULL) ? tokpos : s;

  s=begin;if (!*s) return NULL;
  while ((*s) && (strchr(delim,*s)==NULL)) s++;
  if (*s) tokpos=s+1; else tokpos=s;
  *s=0;
  
  return begin;
}  

char *strcat(char *dest, const char *src)

{
  char *pos=dest;
  
  while (*pos) pos++;
  while (*src) *pos++=*src++;
  *pos=0;
  
  return dest;
}

