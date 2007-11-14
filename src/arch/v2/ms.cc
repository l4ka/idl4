#include "globals.h"
#include "cast.h"
#include "arch/v2.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)
#define dprint(a...) do { if (debug_mode&DEBUG_MARSHAL) print(a); } while (0)

CMSConnection *CMSService2::getConnection(int numChannels, int fid, int iid)

{
  CMSConnection *result = buildConnection(numChannels, fid, iid);
  connections->add(result);
  
  return result;
}

CMSConnection *CMSService2::buildConnection(int numChannels, int fid, int iid)

{
  return new CMSConnection2(this, numChannels, fid, iid);
}

void CMSConnection2::setOption(int option)

{
  /* Fiasco does not support fastcalls */

  if (option==OPTION_ONEWAY)
    this->options |= option;
}

CASTAsmConstraint *CMSService2::getThreadidInConstraints(CASTExpression *rvalue)

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

const char *CMSConnection2::getIPCBindingName(bool sendOnly)

{ 
  if (sendOnly)
    return "l4_i386_ipc_send";
    else return "l4_i386_ipc_call";
}

CASTDeclaration *CMSConnection2::buildWrapperParams(CASTIdentifier *key)

{
  CBEType *mwType = msFactory->getMachineWordType();
  CASTIdentifier *unionIdentifier = key->clone();
  CASTDeclaration *result = NULL;
  
  unionIdentifier->addPrefix("_param_");
  addTo(result, new CASTDeclaration(
    new CASTTypeSpecifier(new CASTIdentifier("CORBA_Object")),
    new CASTDeclarator(new CASTIdentifier("_caller"))));
  addTo(result, new CASTDeclaration(
    new CASTTypeSpecifier(unionIdentifier->clone()),
    new CASTDeclarator(new CASTIdentifier("msgbuf"), new CASTPointer()))
  );  
  addTo(result, mwType->buildDeclaration(
    new CASTDeclarator(new CASTIdentifier("w0"), new CASTPointer())));
  addTo(result, mwType->buildDeclaration(
    new CASTDeclarator(new CASTIdentifier("w1"), new CASTPointer())));
    
  return result;
}

