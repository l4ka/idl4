#include "ops.h"

CASTDeclaration *CBEObjectType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound)

{
  return new CASTDeclaration(new CASTTypeSpecifier("CORBA_Object"), decl, compound);
}
