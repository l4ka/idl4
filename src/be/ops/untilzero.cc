#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

CBEOpCopyUntilZero::CBEOpCopyUntilZero(CMSConnection *connection, int channels, CASTExpression *rvalue, CBEType *type, int sizeBound, const char *name, CBEParameter *param, int flags) : CBEMarshalOp(connection, channels, rvalue, param, flags)

{
  int typSize = (sizeBound==UNKNOWN) ? UNKNOWN : (sizeBound * type->getFlatSize()) / 2;
  int maxSize = (sizeBound==UNKNOWN) ? UNKNOWN : (sizeBound * type->getFlatSize());
  this->chunk = connection->getVariableCopyChunk(channels, 0, typSize, maxSize, name, type);
  this->clientInLengthVar = NULL;
  this->clientOutLengthVar = NULL;
  this->serverBufferVar = NULL;
  this->serverLengthVar = NULL;
  this->elementType = type;

  dprintln("CopyUntilZero (chunk %d)", chunk);
}

void CBEOpCopyUntilZero::buildClientDeclarations(CBEVarSource *varSource)

{
  CBEType *mwType = msFactory->getMachineWordType();
  
  if (HAS_IN(channels))
    clientInLengthVar = varSource->requestVariable(true, mwType);
  if (HAS_OUT(channels))
    clientOutLengthVar = varSource->requestVariable(true, mwType);
}

CASTStatement *CBEOpCopyUntilZero::buildClientMarshal()

{
  CASTStatement *result = NULL;

  if (!HAS_IN(channels))
    return NULL;

  assert(clientInLengthVar);
  
  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=", 
      clientInLengthVar->clone(),
      new CASTFunctionOp(
        new CASTIdentifier(mprintf("idl4_strlen%d", elementType->getFlatSize())), 
        rvalue->clone())))
  );
  
  addTo(result, connection->assignVCCSizeAndDataToBuffer(CHANNEL_IN, chunk,
    rvalue->clone(), clientInLengthVar->clone())
  );
  
  return result;
}

CASTStatement *CBEOpCopyUntilZero::buildClientUnmarshal()

{
  CASTStatement *result = NULL;

  if (!HAS_OUT(channels))
    return NULL;

  assert(clientOutLengthVar);
  
  CASTStatement *assign = connection->assignVCCDataFromBuffer(CHANNEL_OUT,
    chunk, rvalue->clone());
  CASTStatement *copy = assign ? dynamic_cast<CASTStatement *>(assign->next)
                               : NULL;
  if (copy)
    copy->remove();
  addTo(result, assign);
  
  addTo(result, connection->assignVCCSizeFromBuffer(CHANNEL_OUT, chunk,
    clientOutLengthVar->clone()));
  
  if (HAS_IN(channels))
    {
      /* This is an INOUT string. We must reallocate buffer space iff
         the out string is larger than the in string (CLM 1.12) */
         
      CASTStatement *realloc = NULL;
      
      addTo(realloc, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("CORBA_free"),
          rvalue->clone()))
      );
      
      addTo(realloc, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          rvalue->clone(), 
          elementType->buildBufferAllocation(clientOutLengthVar->clone())))
      );
         
      addTo(result, new CASTIfStatement(
        new CASTBinaryOp("<",
          clientInLengthVar->clone(), 
          clientOutLengthVar->clone()),
        new CASTCompoundStatement(realloc))
      );
    } else {
             /* This is an OUT string. We must allocate buffer space
                in any case */
    
             addTo(result, new CASTExpressionStatement(
               new CASTBinaryOp("=",
                 rvalue->clone(), 
                 elementType->buildBufferAllocation(clientOutLengthVar->clone())))
             );
           }
  
  if (copy)
    addTo(result, copy);
  
  return result;  
}

void CBEOpCopyUntilZero::buildServerDeclarations(CBEVarSource *varSource)

{
  CBEType *mwType = msFactory->getMachineWordType();
  serverBufferVar = varSource->requestVariable(true, elementType, 1);
  if (channels&CHANNEL_MASK(CHANNEL_OUT))
    serverLengthVar = varSource->requestVariable(true, mwType, 0);
}

CASTStatement *CBEOpCopyUntilZero::buildServerUnmarshal()

{
  CASTStatement *result = NULL;

  assert(serverBufferVar);

  if (channels&CHANNEL_MASK(CHANNEL_IN))
    {
      addTo(result, connection->assignVCCDataFromBuffer(CHANNEL_IN, chunk,
        serverBufferVar->clone())
      );  
    } else addTo(result, connection->buildVCCServerPrealloc(CHANNEL_OUT, chunk, serverBufferVar->clone()));
  
  return result;
}

CASTExpression *CBEOpCopyUntilZero::buildServerArg(CBEParameter *param)

{
  assert(serverBufferVar);
  
  return serverBufferVar->clone();
}

CASTStatement *CBEOpCopyUntilZero::buildServerMarshal()

{
  CASTStatement *result = NULL;

  if (!(channels&CHANNEL_MASK(CHANNEL_OUT)))
    return NULL;

  assert(serverBufferVar);
  assert(serverLengthVar);

  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=", 
      serverLengthVar->clone(),
      new CASTFunctionOp(
        new CASTIdentifier(mprintf("idl4_strlen%d", elementType->getFlatSize())), 
        serverBufferVar->clone())))
  );
  
  addTo(result, connection->assignVCCSizeAndDataToBuffer(CHANNEL_OUT, chunk,
    serverBufferVar->clone(), serverLengthVar->clone())
  );

  if (!(flags&OP_PROVIDEBUFFER))
    addTo(result, new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("CORBA_free"),
        serverBufferVar->clone()))
    );
  
  return result;
}

CASTStatement *CBEOpCopyUntilZero::buildServerReplyMarshal()

{
  CASTStatement *result = NULL;

  if (!HAS_OUT(channels))
    return NULL;

  assert(serverLengthVar);
  
  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=", 
      serverLengthVar->clone(),
      new CASTFunctionOp(
        new CASTIdentifier(mprintf("idl4_strlen%d", elementType->getFlatSize())), 
        rvalue->clone())))
  );
  
  addTo(result, connection->assignVCCSizeAndDataToBuffer(CHANNEL_OUT, chunk,
    rvalue->clone(), serverLengthVar->clone())
  );
  
  if (!(flags&OP_PROVIDEBUFFER))
    addTo(result, new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("CORBA_free"),
        rvalue->clone()))
    );
  
  return result;
}

void CBEOpCopyUntilZero::buildServerReplyDeclarations(CBEVarSource *varSource)

{
  CBEType *mwType = msFactory->getMachineWordType();
  if (channels&CHANNEL_MASK(CHANNEL_OUT))
    serverLengthVar = varSource->requestVariable(true, mwType, 0);
}

