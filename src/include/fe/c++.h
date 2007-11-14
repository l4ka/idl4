#ifndef __fe_cpp_h__
#define __fe_cpp_h__

#include "fe/compiler.h"
#include "cast.h"

class CPPInputContext : public InputContext

{
public:
  CPPInputContext(char *filename) : InputContext(filename) {};
  CASTBase *Parse();
};

extern CPPInputContext *currentCPPContext;
extern CASTBase *cpTree;

#endif /* defined(__fe_cpp_h__) */
