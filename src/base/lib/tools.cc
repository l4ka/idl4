#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "base.h"

int CBase::indentLevel = 0;
bool CBase::doIndent = true;
bool CBase::doLinebreaks = true;
int CBase::stringRemaining = 0;
char *CBase::stringBuffer = NULL;
int CBase::charsInLine = 0;
FILE *CBase::stream = stderr;
char buffer[20000];

void CBase::putSingle(char c)

{
  if (stringRemaining)
    {
      *stringBuffer++ = c;
      stringRemaining--;
    } else fputc(c, stream);
  charsInLine++;
  if (c=='\n')
    charsInLine = 0;
}  

void CBase::println(const char *fmt, ...)

{
  va_list vl;

  va_start(vl, fmt);
  if (doIndent)
    for (int i=0;i<2*indentLevel;i++) 
      putSingle(' ');
  if (stringRemaining)
    {
      int printed = vsnprintf(stringBuffer, stringRemaining, fmt, vl);
      assert(printed<=stringRemaining);
      stringRemaining -= printed;
      stringBuffer += printed;
    } else vfprintf(stream, fmt, vl);
  if (!doLinebreaks)
    {
      putSingle(' ');
      putSingle('\\');
    }  
  putSingle('\n');
  doIndent = true;
  va_end(vl);
}

void CBase::flushLine()

{
  if (!doIndent)
    println();
}  

void CBase::println()

{
  if (!doLinebreaks)
    putSingle('\\');
  putSingle('\n');
  doIndent = true;
}  

void CBase::print(const char *fmt, ...)

{
  va_list vl;

  va_start(vl, fmt);
  if (doIndent)
    for (int i=0;i<2*indentLevel;i++) 
      putSingle(' ');
  if (stringRemaining)
    {
      int printed = vsnprintf(stringBuffer, stringRemaining, fmt, vl);
      assert(printed<=stringRemaining);
      stringRemaining -= printed;
      stringBuffer += printed;
      charsInLine += printed;
    } else charsInLine += vfprintf(stream, fmt, vl);
  doIndent = false;
  va_end(vl);
}

void CBase::enableStringMode(char *buf, int maxLen)

{
  assert(!stringRemaining);
  stringBuffer = buf;
  stringRemaining = maxLen - 1;
}

void CBase::disableStringMode()

{
  assert(stringRemaining);
  *stringBuffer = 0;
  stringRemaining = 0;
}

void CBase::flush()

{
  fflush(stream);
}

void panic(const char* fmt, ...)

{
  va_list vl;

  /* make sure all output is written before panic message */
  CBase::flush();

  va_start(vl, fmt);
  vfprintf(stderr, "Panic: ", vl);
  vfprintf(stderr, fmt, vl);
  va_end(vl);
  fputc('\n', stderr);
    
  exit(1);
}

void CBase::tabstop(int pos)

{
  while (charsInLine<pos)
    putSingle(' ');      
}

void warning(const char* fmt, ...)

{
  va_list vl;

  va_start(vl, fmt);
  vfprintf(stderr, "Warning: ", vl);
  vfprintf(stderr, fmt, vl);
  va_end(vl);
  fputc('\n', stderr);
}

char *mprintf(const char *fmt, ...)

{
  char *name;
  va_list vl;
  int len;

  va_start(vl, fmt);
  len = vsnprintf(buffer, sizeof(buffer)-1, fmt, vl);
  assert(len>=0);
  assert(len<(int)(sizeof(buffer)-1));
  buffer[len]=0;

  name = (char*)malloc(len+1);
  assert(name!=NULL);
  strcpy(name, buffer);
  
  return name;
}

char *aprintf(const char *fmt, ...)

{
  char *name;
  va_list vl;
  int len;

  va_start(vl, fmt);
  len = vsnprintf(buffer, sizeof(buffer)-1, fmt, vl);
  assert(len>=0);
  assert(len<(int)(sizeof(buffer)-6));
  while (len<35)
    buffer[len++] = ' ';
  buffer[len++] = '\\';
  buffer[len++] = 'n';
  buffer[len++] = '\\';
  buffer[len++] = 't';
  buffer[len]=0;

  name = (char*)malloc(len+1);
  assert(name!=NULL);
  strcpy(name, buffer);
  
  return name;
}

void mfree(char *str)

{
  free(str);
}
