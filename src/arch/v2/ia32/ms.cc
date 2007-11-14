#include "ms.h"
#include "arch/v2.h"

void CMSConnection2I::addServerDopeExtensions()

{
  dopes[CHANNEL_IN]->addWithPrio(
    new CMSChunkX(++chunkID, "_esp", 4, 4, CHUNK_ALIGN4, 
      new CBEOpaqueType("int", 4, 4), 
      NULL, CHANNEL_MASK(CHANNEL_IN), CHUNK_ALLOC_SERVER | CHUNK_PINNED, -2), 
    PRIO_DOPE
  );
  dopes[CHANNEL_IN]->addWithPrio(
    new CMSChunkX(++chunkID, "_ebp", 4, 4, CHUNK_ALIGN4, 
      new CBEOpaqueType("int", 4, 4), 
      NULL, CHANNEL_MASK(CHANNEL_IN), CHUNK_ALLOC_SERVER | CHUNK_PINNED, -1), 
    PRIO_DOPE
  );
}

CMSConnection *CMSService2I::getConnection(int numChannels, int fid, int iid)

{
  CMSConnection *result = buildConnection(numChannels, fid, iid);
  connections->add(result);
  
  return result;
}

CMSConnection *CMSService2I::buildConnection(int numChannels, int fid, int iid)

{
  return new CMSConnection2I(this, numChannels, fid, iid);
}

void CMSConnection2I::setOption(int option)

{
  /* Fiasco does not support fastcalls */

  if (option==OPTION_ONEWAY)
    this->options |= option;
}

CASTAsmConstraint *CMSService2I::getThreadidInConstraints(CASTExpression *rvalue)

{
  CASTAsmConstraint *result = NULL;
  
  addTo(result, new CASTAsmConstraint("S", 
    new CASTBinaryOp(".",
      new CASTBinaryOp(".",
        rvalue,
        new CASTIdentifier("lh")),
      new CASTIdentifier("low")))
  );
      
  addTo(result, new CASTAsmConstraint("D", 
    new CASTBinaryOp(".",
      new CASTBinaryOp(".",
        rvalue->clone(),
        new CASTIdentifier("lh")),
      new CASTIdentifier("high")))
  );
    
  return result;
}

CASTDeclaration *CMSConnection2I::buildWrapperParams(CASTIdentifier *key)

{
  CBEType *mwType = msFactory->getMachineWordType();
  CASTIdentifier *unionIdentifier = key->clone();
  CASTDeclaration *result = NULL;
  
  unionIdentifier->addPrefix("_param_");
  addTo(result, new CASTDeclaration(
    new CASTTypeSpecifier(new CASTIdentifier("l4_threadid_t")),
    new CASTDeclarator(new CASTIdentifier("_caller"))));
  addTo(result, mwType->buildDeclaration(
    new CASTDeclarator(new CASTIdentifier("w0"))));
  addTo(result, new CASTDeclaration(
    new CASTTypeSpecifier(unionIdentifier->clone()),
    new CASTDeclarator(new CASTIdentifier("_par")))
  );  
    
  return result;
}
