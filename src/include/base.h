#ifndef __base_h__
#define __base_h__

#include <stdio.h>

#define DEBUG_TESTSUITE	(1<<0)
#define DEBUG_CPP	(1<<1)
#define DEBUG_AOI	(1<<2)
#define DEBUG_CHECK	(1<<3)
#define DEBUG_CAST	(1<<4)
#define DEBUG_MARSHAL	(1<<5)
#define DEBUG_GENERATOR	(1<<6)
#define DEBUG_IMPORT	(1<<7)
#define DEBUG_FIDS	(1<<8)
#define DEBUG_REORDER	(1<<9)
#define DEBUG_PARANOID	(1<<10)
#define DEBUG_VISUAL	(1<<11)

extern int debug_mode;

class CBase

{
private:
  void putSingle(char c);

protected:
  static char *stringBuffer;
  static int indentLevel;
  static int stringRemaining;
  static int charsInLine;
  static bool doLinebreaks;
  static bool doIndent;
  static FILE *stream;
  
  void indent(int offset) { indentLevel += offset; };
  void enableLinebreaks() { doLinebreaks = true; };
  void disableLinebreaks() { doLinebreaks = false; };
  void println(const char *fmt, ...);
  void print(const char *fmt, ...);
  void println();
  void flushLine();
  void tabstop(int pos);
  
public:
  void enableStringMode(char *buf, int maxLen);
  void disableStringMode();
  static void flush();
};

void panic(const char* fmt, ...) __attribute__ ((noreturn));
void warning(const char* fmt, ...);
char *mprintf(const char *fmt, ...);
char *aprintf(const char *fmt, ...);
void mfree(char *fmt);
const char *getBuiltinMacro(int nr);

#endif
