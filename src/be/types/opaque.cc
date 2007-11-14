#include "ops.h"

CASTDeclaration *CBEOpaqueType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound)

{
  return new CASTDeclaration(new CASTTypeSpecifier(mprintf(name)), decl, compound);
}

int CBEOpaqueType::getArgIndirLevel(CBEDeclType declType)

{ 
  return 0;
}
