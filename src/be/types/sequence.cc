#include "ops.h"

CASTDeclaration *CBESequenceType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound)

{ 
  CASTDeclarationStatement *members = NULL;
  CBEType *u32Type = getType(aoiRoot->lookupSymbol("#u32", SYM_TYPE));
  CBEType *elementType = getType(aoi->elementType);

  addTo(members, new CASTDeclarationStatement(u32Type->buildDeclaration(knitDeclarator("_maximum"))));
  addTo(members, new CASTDeclarationStatement(u32Type->buildDeclaration(knitDeclarator("_length"))));
  addTo(members, new CASTDeclarationStatement(elementType->buildDeclaration(knitIndirectDeclarator("_buffer"))));
                 
  return new CASTDeclaration(
      new CASTAggregateSpecifier("struct", ANONYMOUS, members), decl, compound
  );
}

int CBESequenceType::getArgIndirLevel(CBEDeclType declType)

{ 
  if (declType==OUT)  
    return 1; /* nonstandard */
  
  if ((declType==IN) || (declType==INOUT) || (declType==RETVAL))
    return 1;
    
  return 0;
}

CBEMarshalOp *CBESequenceType::buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags)

{
  CBEMarshalOp *result = NULL;

  CBEMarshalOp *allocOp = new CBEOpAllocOnServer(
    connection, originalType, param, flags
  );

  addTo(result, allocOp);
  
  addTo(result, new CBEOpCopySubArray(connection, channels, allocOp,
    rvalue, getType(aoi->elementType), 
    new CASTIdentifier("_length"), new CASTIdentifier("_maximum"),
    new CASTIdentifier("_buffer"), name, param, flags)
  );
  
  
  return result;
}

CASTStatement *CBESequenceType::buildDefinition()

{
  return NULL;
}

static CASTStatement *buildSequenceInit(CASTExpression *globalPath, CASTExpression *localPath, CBEType *elementType, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, int lengthBound, bool isServer)

{
  CASTIdentifier *loopVar = varSource->requestVariable(false, msFactory->getMachineWordType());
  CASTStatement *result = NULL;

  assert(localPath && globalPath);

  CASTExpression *lengthExpr = new CASTBrackets(
    new CASTBinaryOp("%",
      new CASTFunctionOp(
        new CASTIdentifier("random")),
        new CASTIntegerConstant(lengthBound))
  );
  
  CASTExpression *localEffectivePath;
  if (isServer)
    localEffectivePath = localPath;
    else localEffectivePath = knitPathInstance(globalPath, "client");
    
  CASTExpression *globalEffectivePath;
  if (isServer)
    globalEffectivePath = knitPathInstance(globalPath, "svr_out");
    else globalEffectivePath = knitPathInstance(globalPath, "svr_in");
  
  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      new CASTBinaryOp(".",
        localEffectivePath->clone(),
        new CASTIdentifier("_length")),
      new CASTBinaryOp("=",
        new CASTBinaryOp(".",
          globalEffectivePath->clone(),
          new CASTIdentifier("_length")),
        lengthExpr)))
  );

  CASTExpression *sizeExpr = new CASTBinaryOp(".",
    localEffectivePath->clone(),
    new CASTIdentifier("_length")
  );
  
  if (!isServer && !useMallocFor(param) && !isServer && (type==INOUT))
    sizeExpr = new CASTIntegerConstant(lengthBound);

  CASTExpression *allocAssignment = new CASTBinaryOp("=",
    new CASTBinaryOp(".",
      globalEffectivePath->clone(),
      new CASTIdentifier("_buffer")),
    elementType->buildTypeCast(1,
      new CASTFunctionOp(
        new CASTIdentifier("malloc"),
        new CASTBinaryOp("*",
          sizeExpr,
          new CASTFunctionOp(
            new CASTIdentifier("sizeof"),
            new CASTDeclarationExpression(
              elementType->buildDeclaration(NULL))))))
  );
  
  if (!isServer)
    allocAssignment = new CASTBinaryOp("=",
      new CASTBinaryOp(".",
        localEffectivePath->clone(),
        new CASTIdentifier("_buffer")),
      allocAssignment
    );
    
  addTo(result, new CASTExpressionStatement(allocAssignment));  

  if (!isServer)
    {
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            localEffectivePath->clone(),
            new CASTIdentifier("_maximum")),
          new CASTBinaryOp("=",
            new CASTBinaryOp(".",
              globalEffectivePath->clone(),
              new CASTIdentifier("_maximum")),
            sizeExpr->clone())))
      );
    }  
  
  /* must alloc *_buffer, init _length, _maximum */

  CASTExpression *globalBufferPath = new CASTIndexOp(
    new CASTBinaryOp(".",
      globalPath, 
      new CASTIdentifier("_buffer")),
    loopVar->clone()
  );
  CASTExpression *localBufferPath = new CASTIndexOp(
    new CASTBinaryOp(".",
      localPath, 
      new CASTIdentifier("_buffer")),
    loopVar->clone()
  );
  
  addTo(result, new CASTForStatement(
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        loopVar->clone(),
        new CASTIntegerConstant(0))),
    new CASTBinaryOp("<",
      loopVar->clone(),
      msFactory->getMachineWordType()->buildTypeCast(0,
        new CASTBinaryOp(".",
          localEffectivePath->clone(),
          new CASTIdentifier("_length")))),
    new CASTPostfixOp("++",
      loopVar->clone()),
    knitBlock(
      isServer ? 
      elementType->buildTestServerInit(
        globalBufferPath,
        localBufferPath,
        varSource, type, param, true) :
      elementType->buildTestClientInit(
        globalBufferPath,
        localBufferPath,
        varSource, type, param)))
  );    
  
  return result;
}

CASTStatement *CBESequenceType::buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param)

{
  int lengthBound = (aoi->maxLength==UNKNOWN) ? SERVER_BUFFER_SIZE/getType(aoi->elementType)->getFlatSize() : aoi->maxLength;
  
  if (type==IN || type==INOUT)
    return buildSequenceInit(globalPath, localPath, getType(aoi->elementType), varSource, type, param, lengthBound, false);
    
  if (type==OUT)
    {
      CBEType *elementType = getType(aoi->elementType);
      CASTStatement *result = NULL;
      
      if (useMallocFor(param))
        return NULL;
      
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            knitPathInstance(globalPath, "client"),
            new CASTIdentifier("_buffer")),
          elementType->buildTypeCast(1,
            new CASTFunctionOp(
              new CASTIdentifier("malloc"),
              new CASTBinaryOp("*",
                new CASTIntegerConstant(lengthBound),
                new CASTFunctionOp(
                  new CASTIdentifier("sizeof"),
                  new CASTDeclarationExpression(
                    elementType->buildDeclaration(NULL))))))))
      );
      
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            knitPathInstance(globalPath, "client"),
            new CASTIdentifier("_maximum")),
          new CASTIntegerConstant(lengthBound)))
      );
      
      return result;
    }  
      
  return NULL;
}

CASTStatement *CBESequenceType::buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  CASTStatement *result = NULL;

  if (type==IN)
    addTo(result, new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("free"),
        new CASTBinaryOp(".",
          knitPathInstance(globalPath, "svr_in"),
          new CASTIdentifier("_buffer"))))
    );

  return result;
}

CASTStatement *CBESequenceType::buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{ 
  return NULL;
}

CASTStatement *CBESequenceType::buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable)

{
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  int lengthBound = (aoi->maxLength==UNKNOWN) ? SERVER_BUFFER_SIZE/getType(aoi->elementType)->getFlatSize() : aoi->maxLength;

  return buildSequenceInit(globalPath, localPath, getType(aoi->elementType), varSource, type, param, lengthBound, true);
}

static CASTStatement *buildSequenceCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEType *elementType, CBEVarSource *varSource, CBEDeclType type, bool isServer, char *info)

{
  CBEType *mwType = msFactory->getMachineWordType();
  CASTIdentifier *loopVar = varSource->requestVariable(false, mwType, 0);
  CASTStatement *result = NULL;

  CASTExpression *localEffectivePath;
  if (isServer)
    localEffectivePath = localPath;
    else localEffectivePath = knitPathInstance(globalPath, "client");
    
  CASTExpression *globalEffectivePath;
  if (isServer)
    globalEffectivePath = knitPathInstance(globalPath, "svr_in");
    else globalEffectivePath = knitPathInstance(globalPath, "svr_out");

  addTo(result, new CASTIfStatement(
    new CASTBinaryOp("!=",
      new CASTBinaryOp(".",
        localEffectivePath->clone(),
        new CASTIdentifier("_length")),
      new CASTBinaryOp(".",  
        globalEffectivePath->clone(),
        new CASTIdentifier("_length"))),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(new CASTStringConstant(false, mprintf("%s (length mismatch): %%d!=%%d", info)),
                     new CASTBinaryOp(".",
                       localEffectivePath->clone(),
                       new CASTIdentifier("_length")),
                     new CASTBinaryOp(".",
                       globalEffectivePath->clone(),
                       new CASTIdentifier("_length"))))))
  );

  CASTExpression *globalElementPath = new CASTIndexOp(
    new CASTBinaryOp(".",
      globalPath->clone(), 
      new CASTIdentifier("_buffer")),
    loopVar->clone()
  );
  CASTExpression *localElementPath = new CASTIndexOp(
    new CASTBinaryOp(".",
      localEffectivePath->clone(), 
      new CASTIdentifier("_buffer")),
    loopVar->clone()
  );
  addTo(result, new CASTForStatement(
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        loopVar->clone(),
        new CASTIntegerConstant(0))),
    new CASTBinaryOp("<",
      loopVar->clone(),
      mwType->buildTypeCast(0,
        new CASTBinaryOp(".",
          globalEffectivePath->clone(),
          new CASTIdentifier("_length")))),
    new CASTPostfixOp("++",
      loopVar->clone()),
      knitBlock(
        (isServer) ?
        elementType->buildTestServerCheck(globalElementPath, localElementPath, varSource, type) :
        elementType->buildTestClientCheck(globalElementPath, localElementPath, varSource, type)))
  );
  
  return result;
}

CASTStatement *CBESequenceType::buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  CASTStatement *result = NULL;
  char info[100];

  enableStringMode(info, sizeof(info));
  localPath->write();
  disableStringMode();
  
  addTo(result, buildSequenceCheck(globalPath, localPath, getType(aoi->elementType), varSource, type, false,
    mprintf("Out transfer failed for %s", info))
  );
  
  addTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("free"),
      new CASTBinaryOp(".",
        knitPathInstance(globalPath, "client"),
        new CASTIdentifier("_buffer"))))
  );

  addTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("free"),
      new CASTBinaryOp(".",
        knitPathInstance(globalPath, "svr_out"),
        new CASTIdentifier("_buffer"))))
  );
  
  return result;
}

CASTStatement *CBESequenceType::buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if ((type!=IN) && (type!=INOUT))
    return NULL;

  char info[100];

  enableStringMode(info, sizeof(info));
  localPath->write();
  disableStringMode();
  
  return buildSequenceCheck(globalPath, localPath, getType(aoi->elementType), varSource, type, true,
    mprintf("In transfer failed for %s", info));
}

CASTStatement *CBESequenceType::buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  CASTStatement *result = NULL;

  if (type!=IN)
    return NULL;

  return result;
}

bool CBESequenceType::needsSpecialHandling()

{
  return true;
}  
  
bool CBESequenceType::needsDirectCopy()

{
  return false;  
}

int CBESequenceType::getFlatSize()

{
  return 12;
}

int CBESequenceType::getAlignment()

{
  return globals.word_size;
}

CASTExpression *CBESequenceType::buildDefaultValue()

{
  return new CASTArrayInitializer(new CASTIntegerConstant(0));
}

CASTStatement *CBESequenceType::buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value)

{
  CASTIdentifier *loopVar = varSource->requestVariable(false, msFactory->getMachineWordType());
  CBEType *elementType = getType(aoi->elementType);
  CASTStatement *result = NULL;

  addTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("printf"),
      knitExprList(
        new CASTStringConstant(false, "{ _length: %d _maximum: %d _buffer: "),
        new CASTBinaryOp(".",
          value->clone(),
          new CASTIdentifier("_length")),
        new CASTBinaryOp(".",
          value->clone(),
          new CASTIdentifier("_maximum")))))
  );
          
  addTo(result, new CASTForStatement(
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        loopVar->clone(),
        new CASTIntegerConstant(0))),
    new CASTBinaryOp("<",
      loopVar->clone(),
      msFactory->getMachineWordType()->buildTypeCast(0,
        new CASTBinaryOp(".",
          value->clone(),
          new CASTIdentifier("_length")))),
    new CASTPostfixOp("++",
      loopVar->clone()),
      elementType->buildTestDisplayStmt(
        new CASTBinaryOp(".", globalPath->clone(), new CASTIdentifier("_buffer")),
        new CASTBinaryOp(".", localPath->clone(), new CASTIdentifier("_buffer")),
        varSource, type, 
        new CASTIndexOp(
          new CASTBinaryOp(".", value->clone(), new CASTIdentifier("_buffer")),
          loopVar->clone())))
  );    
  
  return result;
}

bool CBESequenceType::isAllowedParamType()

{
  if (globals.compat_mode)
    return false;
  else
    {
      CBEType *elementType = getType(aoi->elementType);
      return elementType->isAllowedParamType() && !elementType->needsSpecialHandling();
    }
}
