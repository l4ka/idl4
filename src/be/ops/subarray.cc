#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

CBEOpCopySubArray::CBEOpCopySubArray(CMSConnection *connection, int channels, CBEMarshalOp *parentOp, CASTExpression *rvalue, CBEType *type, CASTExpression *lengthSub, CASTExpression *maxSub, CASTExpression *bufferSub, const char *name, CBEParameter *param, int flags) : CBEMarshalOp(connection, channels, rvalue, param, flags)

{
  this->chunk = connection->getVariableCopyChunk(channels, 0, UNKNOWN, UNKNOWN, name, type);
  this->elementType = type;
  this->lengthSub = lengthSub;
  this->maxSub = maxSub;
  this->bufferSub = bufferSub;
  this->parentOp = parentOp;

  dprintln("CopyArray (chunk %d)", chunk);
}

void CBEOpCopySubArray::buildClientDeclarations(CBEVarSource *varSource)

{
}

CASTStatement *CBEOpCopySubArray::buildClientMarshal()

{
  CASTStatement *result = NULL;

  if (HAS_OUT(channels) && !useMallocFor(param))
    connection->provideVCCTargetBuffer(CHANNEL_OUT, chunk,
      new CASTBinaryOp(".",
        rvalue->clone(),
        bufferSub->clone()),
      new CASTBinaryOp("*",
        new CASTBinaryOp(".",
          rvalue->clone(),
          maxSub->clone()),
        new CASTFunctionOp(
          new CASTIdentifier("sizeof"),
          new CASTDeclarationExpression(
            elementType->buildDeclaration(NULL))))
    );

  if (!HAS_IN(channels))
    return NULL;

  addTo(result, connection->assignVCCSizeAndDataToBuffer(CHANNEL_IN, chunk,
    new CASTBinaryOp(".",
      rvalue->clone(), 
      bufferSub->clone()),
    new CASTBinaryOp(".",
      rvalue->clone(),
      lengthSub->clone()))
  );

  return result;
}

CASTStatement *CBEOpCopySubArray::buildClientUnmarshal()

{
  CASTStatement *result = NULL;

  if (!HAS_OUT(channels))
    return NULL;

  if (useMallocFor(param) && HAS_IN(channels))
    addTo(result, new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("CORBA_free"),
        new CASTBinaryOp(".",
          rvalue->clone(),
          new CASTIdentifier("_buffer"))))
    );

  addTo(result, connection->assignVCCSizeFromBuffer(CHANNEL_OUT, chunk,
    new CASTBinaryOp(".",
      rvalue->clone(),
      lengthSub->clone()))
  );

  if (useMallocFor(param))
    {
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            rvalue->clone(),
            maxSub->clone()),
          new CASTBinaryOp(".",  
            rvalue->clone(),
            lengthSub->clone())))
      );

      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            rvalue->clone(),
            bufferSub->clone()), 
          elementType->buildBufferAllocation(
            new CASTBinaryOp(".",
              rvalue->clone(),
              maxSub->clone()))))
      );
  
      addTo(result, connection->assignVCCDataFromBuffer(CHANNEL_OUT, chunk,
        new CASTBinaryOp(".",
          rvalue->clone(),
          bufferSub->clone()))
      );
    }  
  
  return result;  
}

void CBEOpCopySubArray::buildServerDeclarations(CBEVarSource *varSource)

{
}

CASTStatement *CBEOpCopySubArray::buildServerUnmarshal()

{
  CASTStatement *result = NULL;

  if (channels&CHANNEL_MASK(CHANNEL_IN))
    {
      addTo(result, connection->assignVCCDataFromBuffer(CHANNEL_IN, chunk,
        new CASTBinaryOp(".",
          parentOp->buildServerArg(param),
          bufferSub->clone()))
      );
      
      addTo(result, connection->assignVCCSizeFromBuffer(CHANNEL_IN, chunk,
        new CASTBinaryOp(".",
          parentOp->buildServerArg(param),
          lengthSub->clone()))
      );
    } else addTo(result, connection->buildVCCServerPrealloc(CHANNEL_OUT, chunk,
             new CASTBinaryOp(".",
               parentOp->buildServerArg(param),
               bufferSub->clone()))
           );

  return result;
}

CASTExpression *CBEOpCopySubArray::buildServerArg(CBEParameter *param)

{
  return NULL;
}

CASTStatement *CBEOpCopySubArray::buildServerMarshal()

{
  CASTStatement *result = NULL;

  if (!(channels&CHANNEL_MASK(CHANNEL_OUT)))
    return NULL;

  addTo(result, connection->assignVCCSizeAndDataToBuffer(CHANNEL_OUT, chunk,
    new CASTBinaryOp(".",
      parentOp->buildServerArg(param),
      bufferSub->clone()),
    new CASTBinaryOp(".",
      parentOp->buildServerArg(param),
      lengthSub->clone())
    )
  );

  if (!(flags&OP_PROVIDEBUFFER))
    addTo(result, new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("CORBA_free"),
        new CASTBinaryOp(".",
          parentOp->buildServerArg(param),
          bufferSub->clone())))
    );
  
  return result;
}

bool CBEOpCopySubArray::isResponsibleFor(CBEParameter *param)

{
  return false;
}

CASTStatement *CBEOpCopySubArray::buildServerReplyMarshal()

{
  CASTStatement *result = NULL;

  if (!HAS_OUT(channels))
    return NULL;

  addTo(result, connection->assignVCCSizeAndDataToBuffer(CHANNEL_OUT, chunk,
    new CASTBinaryOp(".",
      rvalue->clone(), 
      bufferSub->clone()),
    new CASTBinaryOp(".",
      rvalue->clone(),
      lengthSub->clone()))
  );

  return result;
}
