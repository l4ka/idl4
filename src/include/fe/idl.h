#ifndef __fe_idl_h__
#define __fe_idl_h__

#include <stdio.h>
#include "aoi.h"
#include "compiler.h"

class IDLInputContext : public InputContext

{
  CAoiScope *currentScope;

public:
  CAoiScope *rootScope;

  IDLInputContext(char *filename, CAoiScope *rootScope);

  void Parse();
  CAoiScope *getCurrentScope() { return currentScope; }
  void setCurrentScope(CAoiScope *scope) { currentScope = scope; }
};

extern IDLInputContext *currentIDLContext;
CAoiList *importTypes(const char *filename, CAoiContext *context);

#endif /* defined(__fe_idl_h__) */
