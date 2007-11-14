#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

CBEOpAllocOnServer::CBEOpAllocOnServer(CMSConnection *connection, CBEType *type, CBEParameter *param, int flags) : CBEMarshalOp(connection, 0, NULL, param, flags)

{
  this->type = type;
  this->tempVar = NULL;
}

void CBEOpAllocOnServer::buildServerDeclarations(CBEVarSource *varSource)

{
  tempVar = varSource->requestVariable(true, type);
}

CASTExpression *CBEOpAllocOnServer::buildServerArg(CBEParameter *param)

{
  return tempVar;
}

void CBEOpAllocOnServer::buildClientDeclarations(CBEVarSource *varSource) 

{
}

CASTStatement *CBEOpAllocOnServer::buildClientMarshal()

{ 
  return NULL; 
}

CASTStatement *CBEOpAllocOnServer::buildClientUnmarshal()

{ 
  return NULL; 
}

CASTStatement *CBEOpAllocOnServer::buildServerUnmarshal()

{ 
  return NULL; 
}

CASTStatement *CBEOpAllocOnServer::buildServerMarshal()

{ 
  return NULL; 
}

CASTStatement *CBEOpAllocOnServer::buildServerReplyMarshal()

{
  return NULL;
}

void CBEOpAllocOnServer::buildServerReplyDeclarations(CBEVarSource *varSource)

{
}

