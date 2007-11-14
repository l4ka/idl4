#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

CBEOpSimpleMap::CBEOpSimpleMap(CMSConnection *connection, int channels, CASTExpression *rvalue, CBEType *type, int flatSize, const char *name, CBEParameter *fpageParam, int flags) : CBEMarshalOp(connection, channels, rvalue, fpageParam, flags)

{
  if (flatSize!=4)
    warning("Not implemented: Complex map operation (size: %d)", flatSize);
  
  chunk = connection->getMapChunk(channels, 1, name);
  
  dprintln("Map (chunk %d)", chunk);
}

bool CBEOpSimpleMap::isResponsibleFor(CBEParameter *param)

{
  return (param==this->param);
}

CASTStatement *CBEOpSimpleMap::buildClientUnmarshal()

{ 
  if (!(channels&CHANNEL_MASK(CHANNEL_OUT)))
    return NULL;
    
  return connection->assignFMCFpageFromBuffer(CHANNEL_OUT, chunk, rvalue->clone());
}

CASTExpression *CBEOpSimpleMap::buildServerArg(CBEParameter *param)

{
  int channelInQuestion = HAS_IN(channels) ? CHANNEL_IN : CHANNEL_OUT;

  assert(param==this->param);

  if (HAS_IN(channels))
    return connection->buildFMCFpageTargetExpr(CHANNEL_IN, chunk);
    else return connection->buildFMCFpageSourceExpr(channelInQuestion, chunk);
}

CASTStatement *CBEOpSimpleMap::buildServerMarshal()

{
  if (!HAS_OUT(channels))
    return NULL;
  
  /* This assignment may seem unnecessary at first, but it is required
     for fpages in registers. These are stored in the memory buffer first,
     and the backend needs to build input constraints for them */
    
  return connection->assignFMCFpageToBuffer(CHANNEL_OUT, chunk, 
    connection->buildFMCFpageSourceExpr(CHANNEL_OUT, chunk)
  );
}

CASTStatement *CBEOpSimpleMap::buildClientMarshal()

{ 
  if (!(channels&CHANNEL_MASK(CHANNEL_IN)))
    return NULL;

  /* This assignment may seem unnecessary at first, but it is required
     for fpages in registers. These are stored in the memory buffer first,
     and the backend needs to build input constraints for them */
    
  return connection->assignFMCFpageToBuffer(CHANNEL_IN, chunk, 
    rvalue->clone()
  );
}

void CBEOpSimpleMap::buildClientDeclarations(CBEVarSource *varSource) 

{
}

void CBEOpSimpleMap::buildServerDeclarations(CBEVarSource *varSource)

{
}

CASTStatement *CBEOpSimpleMap::buildServerUnmarshal()

{ 
  return NULL; 
}

CASTStatement *CBEOpSimpleMap::buildServerReplyMarshal()

{
  if (!(channels&CHANNEL_MASK(CHANNEL_OUT)))
    return NULL;

  return connection->assignFMCFpageToBuffer(CHANNEL_OUT, chunk, 
    rvalue->clone()
  );
}
