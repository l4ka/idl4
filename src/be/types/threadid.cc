#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

CBEMarshalOp *CBEThreadIDType::buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags)

{
  CBEMarshalOp *result;

  dprintln("Marshalling %s (%s)", name, aoi->name);
  indent(+1);

  if (globals.compat_mode)
    result = new CBEOpCompatCopy(connection, channels, rvalue, originalType, getFlatSize(), name, param, flags);
  else
    result = new CBEOpSimpleCopy(connection, channels, rvalue, originalType, getFlatSize(), name, param, flags);
  
  indent(-1);
  
  return result;
}

CASTDeclaration *CBEThreadIDType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound)

{
  return new CASTDeclaration(new CASTTypeSpecifier("idl4_threadid_t"), decl, compound);
}

CASTDeclaration *CBEThreadIDType::buildMessageDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound)

{
  if (globals.compat_mode)
    return new CASTDeclaration(new CASTTypeSpecifier("idl4_msg_threadid_t"), decl, compound);
  else
    return buildDeclaration(decl, compound);
}

int CBEThreadIDType::getArgIndirLevel(CBEDeclType declType)

{ 
  if ((declType==INOUT) || (declType==OUT))
    return 1;

  return 0;    
}

CASTStatement *CBEThreadIDType::buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param)

{
  if ((type!=IN) && (type!=INOUT))
    return NULL;

  CASTExpression *value = new CASTFunctionOp(
    new CASTIdentifier("idl4_get_random_threadid")
  );
  
  if (param->aoi->getProperty("handle"))
    value = new CASTIdentifier("server");

  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "svr_in"),
      new CASTBinaryOp("=",
        knitPathInstance(globalPath, "client"),
        value))
  );
}

CASTStatement *CBEThreadIDType::buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  char info[100];

  enableStringMode(info, sizeof(info));
  localPath->write();
  disableStringMode();

  return new CASTIfStatement(
    new CASTUnaryOp("!", 
      new CASTFunctionOp(
        new CASTIdentifier("idl4_threads_equal"),
        knitExprList(
          knitPathInstance(globalPath, "client"),
          knitPathInstance(globalPath, "svr_out")))),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(new CASTStringConstant(false, mprintf("Out transfer fails for %s: %%d!=%%d", info)),
                     knitPathInstance(globalPath, "client")->clone(),
                     knitPathInstance(globalPath, "svr_out")->clone())))
  );
}

CASTStatement *CBEThreadIDType::buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable)

{
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "svr_out"),
      new CASTBinaryOp("=",
        localPath,
        new CASTFunctionOp(
          new CASTIdentifier("idl4_get_random_threadid"))))
  );
}

CASTStatement *CBEThreadIDType::buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if ((type!=IN) && (type!=INOUT))
    return NULL;

  char info[100];

  enableStringMode(info, sizeof(info));
  localPath->write();
  disableStringMode();

  return new CASTIfStatement(
    new CASTUnaryOp("!",
      new CASTFunctionOp(
        new CASTIdentifier("idl4_threads_equal"),
        knitExprList(
          localPath->clone(),
          knitPathInstance(globalPath, "svr_in")))),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(new CASTStringConstant(false, mprintf("In transfer fails for %s: %%d!=%%d", info)),
                     localPath->clone(),
                     knitPathInstance(globalPath, "svr_in"))))
  );
}

CASTStatement *CBEThreadIDType::buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if (type!=IN)
    return NULL;

  char info[100];

  enableStringMode(info, sizeof(info));
  localPath->write();
  disableStringMode();

  return new CASTIfStatement(
    new CASTUnaryOp("!",
      new CASTFunctionOp(
        new CASTIdentifier("idl4_threads_equal"),
        knitExprList(
          localPath->clone(),
          knitPathInstance(globalPath, "svr_in")))),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(new CASTStringConstant(false, mprintf("Output value collision for %s: %%d!=%%d", info)),
                     localPath->clone(),
                     knitPathInstance(globalPath, "svr_in"))))
  );
}

CASTExpression *CBEThreadIDType::buildDefaultValue()

{
  return new CASTIdentifier("idl4_nilthread");
}

CASTStatement *CBEThreadIDType::buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value)

{
  return new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("printf"),
      knitExprList(
        new CASTStringConstant(false, "%t"),
        value))
  );
}

int CBEThreadIDType::getAlignment()

{
  return msFactory->getThreadIDAlignment();
}

int CBEThreadIDType::getFlatSize()

{
  return msFactory->getThreadIDSize();
}

CAoiCustomType *aoiThreadIDType;

CAoiCustomType *createAoiThreadIDType(CAoiScope *parentScope, CAoiContext *context)

{
  if (!aoiThreadIDType)
    aoiThreadIDType = new CAoiCustomType("threadid", msFactory->getThreadIDSize(), parentScope, context);
  return aoiThreadIDType;
}

CBEThreadIDType *threadIDType;

CBEThreadIDType *CMSFactory::getThreadIDType()

{
  if (!threadIDType)
    threadIDType = new CBEThreadIDType(aoiThreadIDType);
  return threadIDType;
}
