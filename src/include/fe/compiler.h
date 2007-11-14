#ifndef __compiler_h__
#define __compiler_h__

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define BUFSIZE 500

class InputContext

{
protected:
  const char **preprocDefines;
  const char **preprocIncludes;
  const char **preprocOptions;
  const char *cppPath;
  FILE *input;
  bool fail;
  bool isPrimaryError;
  int errorcount;
  int warningcount;
  int pipe_end[2];
  pid_t cppPid;

  void ShowErrorPos();

public:
  const char *filename;
  char linebuf[BUFSIZE];
  int lineno;
  int tokenPos;

  InputContext(const char *filename);

  void Error(const char *fmt, ...);
  void Warning(const char *fmt, ...);
  bool hasErrors() { return fail; };
  FILE *getFile() { return input; }
  int getErrorCount() { return errorcount; }
  int getWarningCount() { return warningcount; }
  void NextLine() { lineno++; }
  void rememberError(const char *linebuf, int tokenPos);
  const char *getFilename() { return filename; }
  void setPosition(const char *filename, int lineno) { this->filename = filename; this->lineno = lineno; }
  void update(InputContext *c) { errorcount += c->errorcount; warningcount += c->warningcount; if (c->fail) fail=true; }
  void openPipe();
  void closePipe();
  void setPreprocDefines(const char **defs) { this->preprocDefines = defs; }
  void setPreprocIncludes(const char **incs) { this->preprocIncludes = incs; }
  void setPreprocOptions(const char **opts) { this->preprocOptions = opts; }
  void setCppPath(const char *cppPath) { this->cppPath = cppPath; }
};

class CContext

{
public:
  const char *file, *line;
  int lineNo, pos;
  
  CContext(const char *file, const char *line, int lineNo, int pos)
    { this->file = file; this->line = line; this->lineNo = lineNo; this->pos = pos; };
  void showMessage(const char *error);
};

#endif /* defined(__compiler_h__) */
