#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <errno.h>

#include "fe/idl.h"

extern FILE *idlin;
extern int idlparse();

IDLInputContext *currentIDLContext;

IDLInputContext::IDLInputContext(char *filename, CAoiScope *rootScope) : InputContext(filename)
  
{
  this->rootScope = rootScope;
  this->currentScope = rootScope;
}
  
void IDLInputContext::Parse()
 
{
  openPipe();
  idlin = fdopen(pipe_end[0], "r");
  
  currentIDLContext = this;
  
  if (idlparse())
    Error("Confused by previous errors...");
    
  closePipe();
}
