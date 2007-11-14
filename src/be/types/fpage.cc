#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

CBEMarshalOp *CBEFpageType::buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags)

{
  CBEMarshalOp *result;

  dprintln("Marshalling %s (%s)", name, aoi->name);
  indent(+1);
  result = new CBEOpSimpleMap(connection, channels, rvalue, originalType, 4, name, param, flags);
  indent(-1);
  
  return result;
}

CASTDeclaration *CBEFpageType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound) 

{ 
  return new CASTDeclaration(new CASTTypeSpecifier("idl4_fpage_t"), decl, compound);
}

int CBEFpageType::getArgIndirLevel(CBEDeclType declType)

{ 
  if ((declType==INOUT) || (declType==OUT))
    return 1;
  
  return 0;
}

CASTStatement *CBEFpageType::buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param)

{
  CASTStatement *result = NULL;

  if ((type!=IN) && (type!=INOUT))
    return NULL;

  CASTExpression *randomValue = new CASTFunctionOp(
    new CASTIdentifier("random")
  );

  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "client"),
      new CASTBinaryOp("=",
        knitPathInstance(globalPath, "svr_in"),
        new CASTFunctionOp(
          new CASTIdentifier("idl4_get_sendpage"),
	  randomValue))))
  );
  
  return result;
}

CASTStatement *buildGenericFpageCheck(const char *direction, CASTExpression *expected, CASTExpression *got, const char *info)

{
  CASTStatement *result = NULL;

  addTo(result, new CASTIfStatement(
    new CASTBinaryOp("!=",
      new CASTBinaryOp(".",
        expected->clone(),
      new CASTIdentifier("base")),
      new CASTBinaryOp(".",
        got->clone(),
      new CASTIdentifier("base"))),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(new CASTStringConstant(false, mprintf("%s mapping fails for fpage %s (base mismatch, %%xh!=%%xh)", direction, info)),
                     new CASTUnaryOp("(int)", 
                       new CASTBinaryOp(".", 
                         expected->clone(), 
                       new CASTIdentifier("fpage"))),
                     new CASTUnaryOp("(int)", 
                       new CASTBinaryOp(".",
                         got->clone(),
                       new CASTIdentifier("fpage")))))))
  );

  addTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("idl4_check_rcvpage"),
      got->clone()))
  );
  
  return result;
}

CASTStatement *CBEFpageType::buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  char info[100];

  enableStringMode(info, sizeof(info));
  localPath->write();
  disableStringMode();

  return buildGenericFpageCheck("Out", knitPathInstance(globalPath, "client"), 
    knitPathInstance(globalPath, "svr_out"), info);
}

CASTStatement *CBEFpageType::buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if ((type!=IN) && (type!=INOUT))
    return NULL;

  char info[100];

  enableStringMode(info, sizeof(info));
  localPath->write();
  disableStringMode();

  return buildGenericFpageCheck("In", knitPathInstance(globalPath, "svr_in"), 
    localPath, info);
}

CASTStatement *CBEFpageType::buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  return NULL;
}

CASTStatement *CBEFpageType::buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  return NULL;
}

CASTStatement *CBEFpageType::buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable)

{
  CASTStatement *result = NULL;

  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  CASTExpression *randomValue = new CASTFunctionOp(
    new CASTIdentifier("random")
  );

  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "svr_out"),
      new CASTBinaryOp("=",
        localPath,
        new CASTFunctionOp(
          new CASTIdentifier("idl4_get_sendpage"),
	  randomValue))))
  );
  
  return result;
}

CASTStatement *CBEFpageType::buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  return NULL;
}

CASTStatement *CBEFpageType::buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value)

{
  return new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("printf"),
      knitExprList(
        new CASTStringConstant(false, "fpage(base=%X, page=%X) "),
        new CASTFunctionOp(
          new CASTIdentifier("idl4_fpage_get_base"),
          value),
        new CASTFunctionOp(
          new CASTIdentifier("idl4_raw_fpage"),
          new CASTFunctionOp(
            new CASTIdentifier("idl4_fpage_get_page"),
            value))))
  );
}

CASTExpression *CBEFpageType::buildDefaultValue()

{
  return new CASTCastOp(
    new CASTDeclaration(
      NULL, 
      new CASTDeclarator(
        new CASTIdentifier("idl4_fpage_t"))),
    new CASTArrayInitializer(
      knitExprList(
        new CASTLabeledExpression(
          new CASTIdentifier("base"),
          new CASTIntegerConstant(0)),
        new CASTLabeledExpression(
          new CASTIdentifier("fpage"),
          new CASTIntegerConstant(0)))),
    CASTStandardCast
  );
}
