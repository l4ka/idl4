#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

CBEOpCompatCopy::CBEOpCompatCopy(CMSConnection *connection, int channels, CASTExpression *rvalue, CBEType *type, int flatSize, const char *name, CBEParameter *param, int flags) : CBEMarshalOp(connection, channels, rvalue, param, flags)

{
  chunk = connection->getFixedCopyChunk(channels, flatSize, name, type);
  assert(type->getName());
  elementType = type;
  serverTIDVar = NULL;

  dprintln("CompatCopy (chunk %d)", chunk);
}

void CBEOpCompatCopy::buildClientDeclarations(CBEVarSource *varSource)

{
  if (channels&CHANNEL_MASK(CHANNEL_OUT))
    {
      clientTempVar = varSource->requestVariable(true,
        new CBEOpaqueType(mprintf("idl4_msg_%s_t", elementType->getName()), elementType->getFlatSize(), elementType->getFlatSize(), true),
        0);
    }
}

CASTStatement *CBEOpCompatCopy::buildClientMarshal()

{
  if (channels&CHANNEL_MASK(CHANNEL_IN))
    {
      return connection->assignFCCDataToBuffer(CHANNEL_IN, chunk,
        new CASTFunctionOp(
          new CASTIdentifier(mprintf("idl4_marshal_%s", elementType->getName())),
          knitExprList(rvalue->clone(),
                       new CASTIdentifier("_service"))));
    }    

  return NULL;
}

CASTStatement *CBEOpCompatCopy::buildClientUnmarshal()

{
  CASTStatement *result = NULL;

  if (channels&CHANNEL_MASK(CHANNEL_OUT))
    {
      assert(clientTempVar);

      addTo(result, connection->assignFCCDataFromBuffer(CHANNEL_OUT, chunk,
        clientTempVar->clone()));

      addTo(result,
        new CASTExpressionStatement(new CASTBinaryOp("=", 
          rvalue->clone(),
          new CASTFunctionOp(
            new CASTIdentifier(mprintf("idl4_unmarshal_%s", elementType->getName())),
            knitExprList(clientTempVar->clone(),
                         new CASTIdentifier("_service"))))));
    }
  
  return result;
}

void CBEOpCompatCopy::buildServerDeclarations(CBEVarSource *varSource)

{
  serverTIDVar = varSource->requestVariable(true, elementType, 0);
  if (channels&CHANNEL_MASK(CHANNEL_IN))
    {
      serverTempVar = varSource->requestVariable(true,
        new CBEOpaqueType(mprintf("idl4_msg_%s_t", elementType->getName()), elementType->getFlatSize(), elementType->getFlatSize(), true),
        0);
    }
}

CASTStatement *CBEOpCompatCopy::buildServerUnmarshal()

{
  CASTStatement *result = NULL;

  if (channels&CHANNEL_MASK(CHANNEL_IN))
    {
      assert(serverTempVar);

      addTo(result, connection->assignFCCDataFromBuffer(CHANNEL_IN, chunk,
        serverTempVar->clone()));

      addTo(result,
        new CASTExpressionStatement(new CASTBinaryOp("=", 
          serverTIDVar->clone(),
          new CASTFunctionOp(
            new CASTIdentifier(mprintf("idl4_unmarshal_%s", elementType->getName())),
            knitExprList(serverTempVar->clone(),
                         connection->buildServerCallerID())))));
    }
  
  return result;
}

CASTExpression *CBEOpCompatCopy::buildServerArg(CBEParameter *param)

{
  assert(serverTIDVar);
  
  return serverTIDVar->clone();
}

CASTStatement *CBEOpCompatCopy::buildServerMarshal()

{
  if (channels&CHANNEL_MASK(CHANNEL_OUT))
    {
      return connection->assignFCCDataToBuffer(CHANNEL_OUT, chunk,
        new CASTFunctionOp(
          new CASTIdentifier(mprintf("idl4_marshal_%s", elementType->getName())),
          knitExprList(serverTIDVar->clone(),
                       connection->buildServerCallerID())));
    }    

  return NULL;
}

void CBEOpCompatCopy::buildServerReplyDeclarations(CBEVarSource *varSource)

{
}

CASTStatement *CBEOpCompatCopy::buildServerReplyMarshal()

{
  if (channels&CHANNEL_MASK(CHANNEL_OUT))
    {
      return connection->assignFCCDataToBuffer(CHANNEL_OUT, chunk,
        new CASTFunctionOp(
          new CASTIdentifier(mprintf("idl4_marshal_%s", elementType->getName())),
          knitExprList(rvalue->clone(),
                       new CASTIdentifier("_client"))));
    }    

  return NULL;
}
