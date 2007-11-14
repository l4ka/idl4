#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

CBEOpSimpleCopy::CBEOpSimpleCopy(CMSConnection *connection, int channels, CASTExpression *rvalue, CBEType *type, int flatSize, const char *name, CBEParameter *param, int flags) : CBEMarshalOp(connection, channels, rvalue, param, flags)

{
  chunk = connection->getFixedCopyChunk(channels, flatSize, name, type);
  
  dprintln("SimpleCopy (chunk %d)", chunk);
}

CASTStatement *CBEOpSimpleCopy::buildClientMarshal()

{
  if (!(channels&CHANNEL_MASK(CHANNEL_IN)))
    return NULL;
    
  return connection->assignFCCDataToBuffer(CHANNEL_IN, chunk, rvalue->clone());
}    

CASTStatement *CBEOpSimpleCopy::buildClientUnmarshal()

{
  if (!(channels&CHANNEL_MASK(CHANNEL_OUT)))
    return NULL;

  return connection->assignFCCDataFromBuffer(CHANNEL_OUT, chunk, rvalue->clone());
}    

CASTExpression *CBEOpSimpleCopy::buildServerArg(CBEParameter *param) 

{ 
  CASTExpression *result = NULL;
  
  if (HAS_IN(channels))
    result = connection->buildFCCDataTargetExpr(CHANNEL_IN, chunk);
    else result = connection->buildFCCDataSourceExpr(CHANNEL_OUT, chunk);
    
  assert(result);
    
  return result;  
}

void CBEOpSimpleCopy::buildClientDeclarations(CBEVarSource *varSource) 

{ 
}

void CBEOpSimpleCopy::buildServerDeclarations(CBEVarSource *varSource) 

{ 
}

CASTStatement *CBEOpSimpleCopy::buildServerUnmarshal() 

{
  return NULL;  
}

CASTStatement *CBEOpSimpleCopy::buildServerMarshal() 

{ 
  if (HAS_OUT(channels))
    {
      CASTExpression *sourceExpr;
      if (HAS_IN(channels))
        sourceExpr = connection->buildFCCDataTargetExpr(CHANNEL_IN, chunk);
        else sourceExpr = connection->buildFCCDataSourceExpr(CHANNEL_OUT, chunk);
        
      return connection->assignFCCDataToBuffer(CHANNEL_OUT, chunk, sourceExpr);
    }    

  return NULL; 
}

CASTStatement *CBEOpSimpleCopy::buildServerReplyMarshal()

{
  if (!(channels&CHANNEL_MASK(CHANNEL_OUT)))
    return NULL;
    
  return connection->assignFCCDataToBuffer(CHANNEL_OUT, chunk, rvalue->clone());
}

void CBEMarshalOp::add(CBEMarshalOp *newHead)

{
  if (!newHead)
    return;
  
  CBEMarshalOp *newTail = newHead->prev;
  
  newTail->next = this;
  newHead->prev = this->prev;

  (this->prev)->next = newHead;
  this->prev = newTail;
}
