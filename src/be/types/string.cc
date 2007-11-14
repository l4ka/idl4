#include "ops.h"

CASTDeclaration *CBEStringType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound)

{
  if (decl)
    decl->addIndir(1);

  return getType(aoi->elementType)->buildDeclaration(decl, compound);
}

CBEMarshalOp *CBEStringType::buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags)

{ 
  CAoiProperty *lengthIs = (param) ? param->aoi->getProperty("length_is") : NULL;
  
  if (lengthIs)
    {
      if (lengthIs->constValue)
        {
          CAoiConstInt *intLen = (CAoiConstInt*)lengthIs->constValue;
          return new CBEOpCopyArray(connection, channels, rvalue, 
            getType(aoi->elementType), intLen->value, name, param, flags);
        } else {
                 CBEParameter *parLen = param->getPropertyParameter("length_is");
                 assert(parLen); /* the dependency checker should ensure this */
                 
                 if (parLen->marshalled)
                   semanticError(param->aoi->context, "length parameters cannot be shared");
                   else parLen->marshalled = true;
                 
                 return new CBEOpCopyArray(connection, channels, rvalue, 
                   getType(aoi->elementType), parLen, name, param, flags);
               }
    }

  return new CBEOpCopyUntilZero(connection, channels, rvalue, getType(aoi->elementType), aoi->maxLength, name, param, flags);
}

int CBEStringType::getArgIndirLevel(CBEDeclType declType)

{ 
  if ((declType==INOUT) || (declType==OUT))
    return 1;
    
  return 0;
}

CASTStatement *CBEStringType::buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param)

{
  CAoiProperty *lengthProp = (param!=NULL) ? param->aoi->getProperty("length_is") : NULL;
  CBEType *mwType = msFactory->getMachineWordType();
  CASTStatement *result = NULL;

  int lengthBound = (aoi->maxLength==UNKNOWN) ? SERVER_BUFFER_SIZE/getType(aoi->elementType)->getFlatSize() : aoi->maxLength;
  CASTExpression *lengthExpr;
  if (lengthProp)
    {
      if (lengthProp->refValue)
        {
          lengthExpr = new CASTBinaryOp("-",
            knitPeerInstance(globalPath, "client", mprintf(lengthProp->refValue)),
            new CASTIntegerConstant(1)
          );
        } else lengthExpr = new CASTIntegerConstant(((CAoiConstInt*)lengthProp->constValue)->value - 1);
    } else {
             lengthExpr = new CASTBrackets(
               new CASTBinaryOp("%",
                 new CASTFunctionOp(
                   new CASTIdentifier("random")),
                 new CASTIntegerConstant(lengthBound))
             );
           }  

  if (type==OUT && lengthProp && !useMallocFor(param))
    {
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          knitPathInstance(globalPath, "client"),
          getType(aoi->elementType)->buildTypeCast(1, 
            new CASTFunctionOp(
              new CASTIdentifier("malloc"),
              new CASTIntegerConstant(lengthBound * getType(aoi->elementType)->getFlatSize())))))
      );
      
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          knitPeerInstance(globalPath, "client", mprintf(lengthProp->refValue)),
          new CASTIntegerConstant(lengthBound)))
      );
      
      return result;
    }

  if ((type!=IN) && (type!=INOUT))
    return NULL;

  CASTIdentifier *loopVar = varSource->requestVariable(false, mwType, 0);
  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      loopVar->clone(),
      lengthExpr))
  );
  
  CASTExpression *allocSize = loopVar->clone();

  /* provide space for the trailing zero */
  allocSize = new CASTBinaryOp("+", allocSize, new CASTIntegerConstant(1));  

  if (getType(aoi->elementType)->getFlatSize()>1)
    allocSize = new CASTBinaryOp("*",
      allocSize, new CASTIntegerConstant(getType(aoi->elementType)->getFlatSize())
    );
  
  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "client"),
      getType(aoi->elementType)->buildTypeCast(1, 
        new CASTFunctionOp(
          new CASTIdentifier("malloc"),
          (type==INOUT) ? (CASTExpression*)new CASTIntegerConstant(SERVER_BUFFER_SIZE) : 
            (CASTExpression*)new CASTUnaryOp("(unsigned)", allocSize)))))
  );

  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "svr_in"),
      getType(aoi->elementType)->buildTypeCast(1, 
        new CASTFunctionOp(
          new CASTIdentifier("malloc"),
          new CASTUnaryOp("(unsigned)", allocSize->clone())))))
  );
  
  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=", 
      new CASTIndexOp(
        knitPathInstance(globalPath, "svr_in"),
        loopVar->clone()),
      new CASTBinaryOp("=",
        new CASTIndexOp(
          knitPathInstance(globalPath, "client"),
          loopVar->clone()),
        new CASTIntegerConstant(0))))
  );

  addTo(result, new CASTWhileStatement(
    new CASTBinaryOp(">=",
      new CASTUnaryOp("--", loopVar->clone()),
      new CASTIntegerConstant(0)),
    getType(aoi->elementType)->buildTestClientInit(
      new CASTIndexOp(
        globalPath, 
        loopVar->clone()),
      new CASTIndexOp(
        localPath, 
        loopVar->clone()),
      varSource, type, param))
  );    
  
  return result;        
}

enum CheckType { CHECK_CLIENT, CHECK_SERVER, CHECK_RECHECK };

CASTStatement *buildStringCheck(CBEType *elementType, CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CheckType whichCheck, CBEDeclType type)

{
  CBEType *mwType = msFactory->getMachineWordType();
  CASTIdentifier *loopVar = varSource->requestVariable(false, mwType, 0);
  CASTExpression *globalElementPath = new CASTIndexOp(globalPath->clone(), loopVar->clone());
  CASTExpression *localElementPath = new CASTIndexOp(localPath->clone(), loopVar->clone());
  CASTStatement *elementCheck;
  
  switch (whichCheck)
    {
      case CHECK_CLIENT : elementCheck = elementType->buildTestClientCheck(globalElementPath, localElementPath, varSource, type);
                          break;
      case CHECK_SERVER : elementCheck = elementType->buildTestServerCheck(globalElementPath, localElementPath, varSource, type);
                          break;
      case CHECK_RECHECK: elementCheck = elementType->buildTestServerRecheck(globalElementPath, localElementPath, varSource, type);
                          break;
      default		: elementCheck = NULL;
    }

  return new CASTForStatement(
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        loopVar->clone(),
        new CASTIntegerConstant(0))),
    new CASTBinaryOp("||",  
      new CASTUnaryOp("!",
        loopVar->clone()),
      new CASTBinaryOp(">",
        new CASTIndexOp(
          knitPathInstance(globalPath, (whichCheck==CHECK_CLIENT) ? "svr_out" : "svr_in"),
          new CASTBinaryOp("-",
            loopVar->clone(),
            new CASTIntegerConstant(1))),
        new CASTIntegerConstant(0))),
    new CASTPostfixOp("++",
      loopVar->clone()),
      elementCheck
  );
}

CASTStatement *CBEStringType::buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  CASTStatement *result = NULL;
  
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  addTo(result, buildStringCheck(
    getType(aoi->elementType), globalPath, localPath, varSource, CHECK_CLIENT, type)
  );

  if ((type==INOUT) || (type==OUT))
    addTo(result, new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("free"),
        knitPathInstance(globalPath, "svr_out")))
    );

  return result;
}

CASTStatement *CBEStringType::buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  CASTStatement *result = NULL;

  if (type==OUT)
    addTo(result, new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("free"),
        knitPathInstance(globalPath, "client")))
    );

  return result;
}

CASTStatement *CBEStringType::buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  CASTStatement *result = NULL;

  if ((type==IN) || (type==INOUT))
    {
      addTo(result, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("free"),
          knitPathInstance(globalPath, "client")))
      );

      addTo(result, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("free"),
          knitPathInstance(globalPath, "svr_in")))
      );
    }  

  return result;
}

CASTStatement *CBEStringType::buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable)

{
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  CBEType *mwType = msFactory->getMachineWordType();
  CASTIdentifier *loopVar = varSource->requestVariable(false, mwType, 0);
  CASTStatement *result = NULL;

  CAoiProperty *lengthProp = (param!=NULL) ? param->aoi->getProperty("length_is") : NULL;
  CASTExpression *lengthExpr;
  
  if (lengthProp)
    {
      if (lengthProp->refValue)
        {
          lengthExpr = new CASTBinaryOp("-",
            knitPeerInstance(globalPath, "svr_out", mprintf(lengthProp->refValue)),
            new CASTIntegerConstant(1)
          );
        } else lengthExpr = new CASTIntegerConstant(((CAoiConstInt*)lengthProp->constValue)->value - 1);
    } else {
             int lengthBound = (aoi->maxLength==UNKNOWN) ? SERVER_BUFFER_SIZE/getType(aoi->elementType)->getFlatSize() : aoi->maxLength;
     
             lengthExpr = new CASTBrackets(
               new CASTBinaryOp("%",
                 new CASTFunctionOp(
                   new CASTIdentifier("random")),
                 new CASTIntegerConstant(lengthBound))
             );
           }  

  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      loopVar->clone(),
      lengthExpr))
  );

  CASTExpression *allocSize = loopVar->clone();

  /* provide space for the trailing zero */
  allocSize = new CASTBinaryOp("+", allocSize, new CASTIntegerConstant(1));  

  if (getType(aoi->elementType)->getFlatSize()>1)
    allocSize = new CASTBinaryOp("*",
      allocSize, new CASTIntegerConstant(getType(aoi->elementType)->getFlatSize())
    );

  if (!bufferAvailable)
    addTo(result, new CASTExpressionStatement(
      new CASTBinaryOp("=",
        localPath->clone(),
        getType(aoi->elementType)->buildTypeCast(1,
          new CASTFunctionOp(
            new CASTIdentifier("malloc"),
            new CASTUnaryOp("(unsigned)", allocSize)))))
    );

  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "svr_out"),
      getType(aoi->elementType)->buildTypeCast(1,
        new CASTFunctionOp(
          new CASTIdentifier("malloc"),
          new CASTUnaryOp("(unsigned)", allocSize)))))
  );
  
  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=", 
      new CASTIndexOp(
        knitPathInstance(globalPath, "svr_out"),
        loopVar->clone()),
      new CASTBinaryOp("=",
        new CASTIndexOp(
          localPath->clone(),
          loopVar->clone()),
        new CASTIntegerConstant(0))))
  );

  addTo(result, new CASTWhileStatement(
    new CASTBinaryOp(">=",
      new CASTUnaryOp("--", loopVar->clone()),
      new CASTIntegerConstant(0)),
    getType(aoi->elementType)->buildTestServerInit(
      new CASTIndexOp(
        globalPath, 
        loopVar->clone()),
      new CASTIndexOp(
        localPath, 
        loopVar->clone()),
      varSource, type, param, bufferAvailable))
  );    
  
  return result;        
}

CASTStatement *CBEStringType::buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if ((type!=IN) && (type!=INOUT))
    return NULL;

  return buildStringCheck(
    getType(aoi->elementType), globalPath, localPath, varSource, 
    CHECK_SERVER, type
  );
}

CASTStatement *CBEStringType::buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if (type!=IN)
    return NULL;

  return buildStringCheck(
    getType(aoi->elementType), globalPath, localPath, varSource, 
    CHECK_RECHECK, type
  );
}

CASTExpression *CBEStringType::buildDefaultValue()

{
  return new CASTCastOp(
    getType(aoi->elementType)->buildDeclaration(
      new CASTDeclarator((CASTIdentifier*)NULL, new CASTPointer(1))),
    new CASTIdentifier("NULL"),
    CASTStandardCast);
}

CASTStatement *CBEStringType::buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value)

{
  return new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("printf"),
      knitExprList(
        new CASTStringConstant(false, "%s"),
        value))
  );
}
