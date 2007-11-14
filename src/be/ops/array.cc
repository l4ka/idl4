#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

CBEOpCopyArray::CBEOpCopyArray(CMSConnection *connection, int channels, CASTExpression *rvalue, CBEType *type, CBEParameter *lengthParam, const char *name, CBEParameter *param, int flags) : CBEMarshalOp(connection, channels, rvalue, param, flags)

{
  this->chunk = connection->getVariableCopyChunk(channels, 0, UNKNOWN, UNKNOWN, name, type);
  this->elementType = type;
  this->lengthParam = lengthParam;
  this->serverBufferVar = NULL;
  this->serverLengthVar = NULL;
  this->fixedLength = UNKNOWN;

  dprintln("CopyArray (chunk %d)", chunk);
}

CBEOpCopyArray::CBEOpCopyArray(CMSConnection *connection, int channels, CASTExpression *rvalue, CBEType *type, int fixedLength, const char *name, CBEParameter *param, int flags) : CBEMarshalOp(connection, channels, rvalue, param, flags)

{
  this->chunk = connection->getVariableCopyChunk(channels, 0, UNKNOWN, UNKNOWN, name, type);
  this->elementType = type;
  this->lengthParam = NULL;
  this->serverBufferVar = NULL;
  this->serverLengthVar = NULL;
  this->fixedLength = fixedLength;

  dprintln("CopyArray (chunk %d)", chunk);
}

void CBEOpCopyArray::buildClientDeclarations(CBEVarSource *varSource)

{
}

CASTStatement *CBEOpCopyArray::buildClientMarshal()

{
  CASTStatement *result = NULL;

  CASTExpression *lengthExpr;
  if (fixedLength==UNKNOWN)
    {
      assert(lengthParam);
      lengthExpr = lengthParam->buildIdentifier();
      for (int i=0;i<param->getArgIndirLevel();i++)
        lengthExpr = new CASTUnaryOp("*", lengthExpr);
    } else lengthExpr = new CASTIntegerConstant(fixedLength);

  if (!useMallocFor(param) && HAS_OUT(channels))
    addTo(result, connection->provideVCCTargetBuffer(CHANNEL_OUT, chunk,
      rvalue->clone(), 
      new CASTBinaryOp("*",
        lengthExpr->clone(),
        new CASTFunctionOp(
          new CASTIdentifier("sizeof"),
          new CASTDeclarationExpression(
            elementType->buildDeclaration(NULL)))))
    );
  
  if (HAS_IN(channels))
    addTo(result, connection->assignVCCSizeAndDataToBuffer(CHANNEL_IN, chunk,
      rvalue->clone(), lengthExpr->clone())
    );

  return result;
}

CASTStatement *CBEOpCopyArray::buildClientUnmarshal()

{
  CASTStatement *result = NULL;

  if (!HAS_OUT(channels))
    return NULL;

  if (HAS_IN(channels))
    addTo(result, new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("CORBA_free"),
        rvalue->clone()))
    );

  CASTExpression *lengthExpr;
  if (fixedLength==UNKNOWN)
    {
      assert(lengthParam);
      lengthExpr = lengthParam->buildIdentifier();
      for (int i=0;i<param->getArgIndirLevel();i++)
        lengthExpr = new CASTUnaryOp("*", lengthExpr);

      addTo(result, connection->assignVCCSizeFromBuffer(CHANNEL_OUT, chunk,
        lengthExpr->clone()));
    } else lengthExpr = new CASTIntegerConstant(fixedLength);

  if (useMallocFor(param))
    {
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          rvalue->clone(), 
          elementType->buildBufferAllocation(lengthExpr->clone())))
      );
  
      addTo(result, connection->assignVCCDataFromBuffer(CHANNEL_OUT, chunk,
        rvalue->clone())
      );
    }  
  
  return result;  
}

void CBEOpCopyArray::buildServerDeclarations(CBEVarSource *varSource)

{
  serverBufferVar = varSource->requestVariable(true, elementType, 1);
  if (this->lengthParam)
    serverLengthVar = varSource->requestVariable(true, getType(lengthParam->aoi->type));
}

CASTStatement *CBEOpCopyArray::buildServerUnmarshal()

{
  CASTStatement *result = NULL;

  assert(serverBufferVar);

  if (channels&CHANNEL_MASK(CHANNEL_IN))
    {
      addTo(result, connection->assignVCCDataFromBuffer(CHANNEL_IN, chunk,
        serverBufferVar->clone())
      ); 
       
      if (lengthParam)
        {
          assert(serverLengthVar);
          addTo(result, connection->assignVCCSizeFromBuffer(CHANNEL_IN, chunk,
            serverLengthVar->clone())
          );
        }  
    } else addTo(result, connection->buildVCCServerPrealloc(CHANNEL_OUT, chunk, serverBufferVar->clone()));
  
  return result;
}

CASTExpression *CBEOpCopyArray::buildServerArg(CBEParameter *param)

{
  if (param == this->param)
    {
      assert(serverBufferVar);
      return serverBufferVar->clone();
    }
    
  if (param == this->lengthParam)
    {
      assert(serverLengthVar);
      return serverLengthVar->clone();
    }
    
  return NULL;
}

CASTStatement *CBEOpCopyArray::buildServerMarshal()

{
  CASTStatement *result = NULL;

  if (!(channels&CHANNEL_MASK(CHANNEL_OUT)))
    return NULL;

  assert(serverBufferVar);

  CASTExpression *lengthExpr;
  if (fixedLength==UNKNOWN)
    {
      assert(lengthParam);
      lengthExpr = serverLengthVar->clone();
    } else lengthExpr = new CASTIntegerConstant(fixedLength);

  addTo(result, connection->assignVCCSizeAndDataToBuffer(CHANNEL_OUT, chunk,
    serverBufferVar->clone(), 
    lengthExpr
    )
  );

  if (!(flags&OP_PROVIDEBUFFER))
    addTo(result, new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("CORBA_free"),
        serverBufferVar->clone()))
    );
  
  return result;
}

bool CBEOpCopyArray::isResponsibleFor(CBEParameter *param)

{
  return ((param == this->param) || 
          ((this->lengthParam != NULL) && (param == this->lengthParam)));
}

CASTStatement *CBEOpCopyArray::buildServerReplyMarshal()

{
  CASTStatement *result = NULL;

  if (!HAS_OUT(channels))
    return NULL;

  CASTExpression *lengthExpr;
  if (fixedLength==UNKNOWN)
    {
      assert(lengthParam);
      lengthExpr = lengthParam->buildIdentifier();
      for (int i=0;i<param->getArgIndirLevel();i++)
        lengthExpr = new CASTUnaryOp("*", lengthExpr);
    } else lengthExpr = new CASTIntegerConstant(fixedLength);

  addTo(result, connection->assignVCCSizeAndDataToBuffer(CHANNEL_OUT, chunk,
    rvalue->clone(), lengthExpr->clone())
  );

  return result;
}

void CBEOpCopyArray::buildServerReplyDeclarations(CBEVarSource *varSource)

{
}

