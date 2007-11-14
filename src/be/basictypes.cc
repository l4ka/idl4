#include "ops.h"

CASTIdentifier *CBEType::buildIdentifier()

{
  if (!aoi->name)
    return NULL;
    
  if (aoi->flags & FLAG_DONTSCOPE)
    return new CASTIdentifier(aoi->name);
  
  CASTIdentifier *id = knitScopedNameFrom(aoi->parentScope->scopedName);
  char *postfix = mprintf("%s%s", (aoi->parentScope == aoiRoot) ? "" : "_", aoi->name);
  id->addPostfix(postfix);
  mfree(postfix);

  return id;
}

CASTExpression *CBEType::buildTypeCast(int indirLevel, CASTExpression *expr)

{
  CASTPointer *ptr = (indirLevel>0) ? new CASTPointer(indirLevel) : NULL;
  CASTDeclarator *decl = (indirLevel>0) ? new CASTDeclarator((CASTIdentifier*)NULL, ptr) : NULL;
  char buffer[80];
  
  enableStringMode(buffer, sizeof(buffer));
  buildDeclaration(decl)->write();
  disableStringMode();
  
  return new CASTUnaryOp(mprintf("(%s)", buffer), expr);
}

CASTStatement *CBEType::buildConditionalDefinition(CASTStatement *simpleDef)

{
  /* build preprocessor directives around */

  CASTIdentifier *defName = buildIdentifier();
  defName->addPrefix("_typedef___");

  CASTBase *subtree = new CASTPreprocDefine(defName, ANONYMOUS);
  addTo(subtree, simpleDef);
  
  return new CASTPreprocConditional(
    new CASTUnaryOp("!", new CASTFunctionOp(new CASTIdentifier("defined"), defName)), subtree
  );
}

CASTStatement *CBEFixedType::buildDefinition()

{
  if (aoi->flags & FLAG_DONTDEFINE)
    return NULL;

  CASTDeclarationStatement *members = NULL;
  CBEType *u16Type = getType(aoiRoot->lookupSymbol("#u16", SYM_TYPE));
  CBEType *s16Type = getType(aoiRoot->lookupSymbol("#s16", SYM_TYPE));
  CBEType *charType = getType(aoiRoot->lookupSymbol("#s8", SYM_TYPE));

  addTo(members, new CASTDeclarationStatement(u16Type->buildDeclaration(knitDeclarator("_digits"))));
  addTo(members, new CASTDeclarationStatement(s16Type->buildDeclaration(knitDeclarator("_scale"))));

  addTo(members, new CASTDeclarationStatement(charType->buildDeclaration(
    new CASTDeclarator(
      new CASTIdentifier("_value"), NULL,
      new CASTBinaryOp("/", 
        new CASTBinaryOp("+", 
          new CASTIntegerConstant(aoi->totalDigits),
          new CASTIntegerConstant(2)),
        new CASTIntegerConstant(2)))))
  );

  CASTDeclarationSpecifier *spec = new CASTStorageClassSpecifier("typedef");
  addTo(spec, new CASTAggregateSpecifier("struct", ANONYMOUS, members));
  
  CASTStatement *result = NULL;
  
  addTo(result, buildConditionalDefinition(
    new CASTDeclarationStatement(
      new CASTDeclaration(
        spec, 
        new CASTDeclarator(buildIdentifier()))))
  ); 
  
  addTo(result, new CASTSpacer());
  
  return result;
}

CASTDeclaration *CBEFixedType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound) 

{ 
  return new CASTDeclaration(new CASTTypeSpecifier(buildIdentifier()), decl, compound);
}

CASTIdentifier *CBEExceptionType::buildID()

{
  CASTIdentifier *id = knitScopedNameFrom(aoi->parentScope->scopedName);
  char *postfix = mprintf("%s%s", (aoi->parentScope == aoiRoot) ? "" : "_", aoi->name);
  id->addPostfix(postfix);
  mfree(postfix);
  id->addPrefix("ex_");
  return id;
}

CASTDeclaration *CBEExceptionType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound) 

{ 
  return new CASTDeclaration(new CASTTypeSpecifier(buildIdentifier()), decl, compound);
}

CASTStatement *CBEExceptionType::buildDefinition()

{
  if (aoi->flags & FLAG_DONTDEFINE)
    return NULL;
    
  CASTDeclarationStatement *members = NULL;
  CASTStatement *result = NULL;
  
  forAll(aoi->members, addTo(members, new CASTDeclarationStatement(getParameter(item)->buildDeclaration())));
  
  CASTDeclarationSpecifier *spec = new CASTStorageClassSpecifier("typedef");
  addTo(spec, new CASTAggregateSpecifier("struct", buildIdentifier(), members));
  
  addTo(result, new CASTPreprocDefine(
    buildID(),
    new CASTIntegerConstant(id))
  );
  addTo(result, new CASTSpacer());
  
  addTo(result, buildConditionalDefinition(
    new CASTDeclarationStatement(
      new CASTDeclaration(
        spec, 
        new CASTDeclarator(buildIdentifier()))))
  ); 
  
  addTo(result, new CASTSpacer());
  
  return result;
}

int CBEFixedType::getArgIndirLevel(CBEDeclType declType)

{ 
  if ((declType==IN) || (declType==INOUT) || (declType==OUT))
    return 1;
    
  return 0;
}

int CBEExceptionType::getArgIndirLevel(CBEDeclType declType)

{ 
  return 0;
}

CBEMarshalOp *CBEFixedType::buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags)

{ 
  warning("Not implemented: fixed types");
  return new CBEOpDummy(param); 
}

CBEMarshalOp *CBEExceptionType::buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags)

{ 
  return new CBEOpDummy(param); 
}

const char *CBEType::getDirectionString(CBEDeclType type)

{
  switch (type) 
    {
      case IN    : return "in";
      case INOUT : return "inout";
      case OUT   : return "out";
      case RETVAL: return "retval";
      default    : return "???";
    }
}
