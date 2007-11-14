#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <errno.h>

#include "fe/c++.h"

extern FILE *cppin;
extern int cppparse();

CPPInputContext *currentCPPContext;
CASTBase *cpTree = NULL;

CASTBase *CPPInputContext::Parse()
 
{
  openPipe();
  cppin = fdopen(pipe_end[0], "r");
  
  currentCPPContext = this;
  
  if (cppparse())
    Error("Confused by previous errors...");
    
  closePipe();
    
  return cpTree;
}
