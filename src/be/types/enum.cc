#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

CASTDeclaration *CBEEnumType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound) 
 
{ 
  return new CASTDeclaration(new CASTTypeSpecifier(buildIdentifier()), decl, compound);
}

CASTStatement *CBEEnumType::buildDefinition()

{
  CASTStatement *result = NULL;
  
  forAll(aoi->members,
    {
      CAoiRef *ref = (CAoiRef*)item;
      CAoiConstant *cnst = (CAoiConstant*)ref->ref;
      CASTIdentifier *ident = buildIdentifier();
      ident->addPostfix("_");
      ident->addPostfix(cnst->name);
      addTo(result, new CASTPreprocDefine(
        ident, 
        getConstBase(cnst->value)->buildExpression())
      );
    }
  );
  
  CASTDeclaration *declar = new CASTDeclaration(
    new CASTTypeSpecifier(mprintf("CORBA_enum")),
    new CASTDeclarator(getType(aoi)->buildIdentifier())
  );
  
  declar->addSpecifier(new CASTStorageClassSpecifier("typedef"));
  addTo(result, new CASTDeclarationStatement(declar));
  result = buildConditionalDefinition(result);
  addTo(result, new CASTSpacer());
  return result;
}

int CBEEnumType::getArgIndirLevel(CBEDeclType declType)

{ 
  if ((declType==INOUT) || (declType==OUT))
    return 1;
    
  return 0;
}

CBEMarshalOp *CBEEnumType::buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags)

{
  CBEMarshalOp *result;

  dprintln("Marshalling %s (%s)", name, aoi->name);
  indent(+1);

  result = new CBEOpSimpleCopy(connection, channels, rvalue, originalType, 4, name, param, flags);
  
  indent(-1);
  
  return result;
}

CASTStatement *CBEEnumType::buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param)

{
  if ((type!=IN) && (type!=INOUT))
    return NULL;

  CASTExpression *randomValue = new CASTFunctionOp(
    new CASTIdentifier("random")
  );
  
  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "svr_in"),
      new CASTBinaryOp("=",
        knitPathInstance(globalPath, "client"),
        randomValue))
  );
}

CASTStatement *CBEEnumType::buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

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
        knitExprList(new CASTStringConstant(false, mprintf("Out transfer fails for %s: %%d!=%%d", info)),
                     new CASTUnaryOp("(int)", knitPathInstance(globalPath, "client")->clone()),
                     new CASTUnaryOp("(int)", knitPathInstance(globalPath, "svr_out")->clone()))))
  );
}

CASTStatement *CBEEnumType::buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  return NULL;
}

CASTStatement *CBEEnumType::buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  return NULL;
}

CASTStatement *CBEEnumType::buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable)

{
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  CASTExpression *randomValue = new CASTFunctionOp(
    new CASTIdentifier("random")
  );

  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "svr_out"),
      new CASTBinaryOp("=",
        localPath,
        randomValue))
  );
}

CASTStatement *CBEEnumType::buildTestServerCheckGeneric(CASTExpression *globalPath, CASTExpression *localPath, const char *text)

{
  char info[100];

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
        knitExprList(new CASTStringConstant(false, mprintf("%s for %s: %%d!=%%d", text, info)),
                     localPath->clone(),
                     knitPathInstance(globalPath, "svr_in"))))
  );
}

CASTStatement *CBEEnumType::buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if ((type!=IN) && (type!=INOUT))
    return NULL;

  return buildTestServerCheckGeneric(globalPath, localPath, "In transfer fails");
}

CASTStatement *CBEEnumType::buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if (type!=IN)
    return NULL;

  return buildTestServerCheckGeneric(globalPath, localPath, "Output value collision detected");
}

CASTExpression *CBEEnumType::buildDefaultValue()

{
  return new CASTIntegerConstant(0);
}

CASTStatement *CBEEnumType::buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value)

{
  CASTStatement *switchBody = NULL;
  
  forAll(aoi->members,
    {
      CAoiRef *ref = (CAoiRef*)item;
      CAoiConstant *cnst = (CAoiConstant*)ref->ref;
      CASTIdentifier *ident = buildIdentifier();
      ident->addPostfix("_");
      ident->addPostfix(cnst->name);
      addTo(switchBody, new CASTCaseStatement(
        knitStmtList(
          new CASTExpressionStatement(
            new CASTFunctionOp(
              new CASTIdentifier("printf"),
              new CASTStringConstant(false, cnst->name))),
          new CASTBreakStatement()),
        ident)
      );
    }
  );

  return new CASTSwitchStatement(
    value,
    new CASTCompoundStatement(switchBody)
  );
}
