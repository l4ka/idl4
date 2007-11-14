#include "ops.h"
#include "cast.h"

CASTStatement *CBEPointerType::buildDefinition()

{
  if (aoi->flags&FLAG_DONTDEFINE)
    return NULL;

  CASTStatement *subdef = getType(aoi->ref)->buildDefinition();
  subdef->accept(new CBEAddIndirVisitor());
  
  return subdef;
}

CASTDeclaration *CBEPointerType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound) 

{ 
  if (aoi->name)
    {
      CASTDeclarationSpecifier *specifiers = NULL;
      addTo(specifiers, new CASTTypeSpecifier(buildIdentifier()));
      return new CASTDeclaration(specifiers, decl, compound);
    }
 
  CASTDeclaration *subdecl = getType(aoi->ref)->buildDeclaration(decl, compound);
  subdecl->declarators->addIndir(1);
  return subdecl;
}

CBEMarshalOp *CBEPointerType::buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags)

{ 
  return new CBEOpSimpleCopy(connection, channels, rvalue, originalType, getFlatSize(), name, param, flags);
}

int CBEPointerType::getArgIndirLevel(CBEDeclType declType)

{ 
  return (declType==IN || declType==RETVAL) ? 0 : 1;
}

CASTStatement *CBEPointerType::buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param)

{
  CASTStatement *result = NULL;

  if ((type!=IN) && (type!=INOUT))
    return NULL;

  CASTExpression *randomValue = getType(aoi->ref)->buildTypeCast(1,
    new CASTFunctionOp(
      new CASTIdentifier("randomPtr"))
  );
  
  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "svr_in"),
      new CASTBinaryOp("=",
        knitPathInstance(globalPath, "client"),
        randomValue))
  );
}

CASTStatement *CBEPointerType::buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  char info[100];

  enableStringMode(info, sizeof(info));
  localPath->write();
  disableStringMode();

  return new CASTIfStatement(
    new CASTBinaryOp("!=",
      knitPathInstance(globalPath, "client"),
      knitPathInstance(globalPath, "svr_out")),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(new CASTStringConstant(false, mprintf("Out transfer fails for %s: %%p!=%%p", info)),
                     knitPathInstance(globalPath, "client")->clone(),
                     knitPathInstance(globalPath, "svr_out")->clone())))
  );
}

CASTStatement *CBEPointerType::buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  return NULL;
}

CASTStatement *CBEPointerType::buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  return NULL;
}

CASTStatement *CBEPointerType::buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable)

{
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  CASTExpression *randomValue = getType(aoi->ref)->buildTypeCast(1,
    new CASTFunctionOp(
      new CASTIdentifier("randomPtr"))
  );

  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "svr_out"),
      new CASTBinaryOp("=",
        localPath,
        randomValue))
  );
}

CASTStatement *CBEPointerType::buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  char info[100];

  if ((type!=IN) && (type!=INOUT))
    return NULL;

  enableStringMode(info, sizeof(info));
  localPath->write();
  disableStringMode();

  return new CASTIfStatement(
    new CASTBinaryOp("!=",
      localPath->clone(),
      knitPathInstance(globalPath, "svr_in")),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(new CASTStringConstant(false, mprintf("In transfer fails for %s: %%p!=%%p", info)),
                     localPath->clone(),
                     knitPathInstance(globalPath, "svr_in"))))
  );
}

CASTStatement *CBEPointerType::buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  char info[100];

  if (type!=IN)
    return NULL;

  enableStringMode(info, sizeof(info));
  localPath->write();
  disableStringMode();

  return new CASTIfStatement(
    new CASTBinaryOp("!=",
      localPath->clone(),
      knitPathInstance(globalPath, "svr_in")),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(new CASTStringConstant(false, mprintf("Output value collision for %s: %%p!=%%p", info)),
                     localPath->clone(),
                     knitPathInstance(globalPath, "svr_in"))))
  );
}

CASTExpression *CBEPointerType::buildDefaultValue()

{
  return getType(aoi->ref)->buildTypeCast(1, new CASTIdentifier("NULL"));
}

CASTStatement *CBEPointerType::buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value)

{
  return new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("printf"),
      knitExprList(
        new CASTStringConstant(false, "%%p"),
        value))
  );
}

void CBEAddIndirVisitor::visit(CASTStorageClassSpecifier *peer)

{
  if (!strcmp(peer->keyword, "typedef"))
    seenTypedef = true;
}

void CBEAddIndirVisitor::visit(CASTDeclarator *peer)

{
  if (seenTypedef)
    {
      peer->addIndir(1);
      seenTypedef = false;
    }  
}

CASTStatement *CBEPointerType::buildAssignment(CASTExpression *dest, CASTExpression *src)

{
  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      dest,
      getType(aoi->ref)->buildTypeCast(1, src))
  );
}
