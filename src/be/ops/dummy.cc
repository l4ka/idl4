#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

CBEOpDummy::CBEOpDummy(CBEParameter *param) : CBEMarshalOp(NULL, 0, NULL, param, 0) 

{
  /* As the build functions will mostly be called without parameters,
     the constructor should receive all the information necessary to
     complete the operation */
}

void CBEOpDummy::buildClientDeclarations(CBEVarSource *varSource) 

{
  /* Use the varSource to allocate any temporary variables you will need
     on the client side (e.g. an integer to hold the string length) */
}

CASTStatement *CBEOpDummy::buildClientMarshal()

{ 
  /* Copy the element into the marshal buffer. Code returned here will be
     inserted on the client side before IPC is invoked. Do not assume that
     this function has already been called for other elements; the compiler
     is free to use any order */

  return NULL; 
}

CASTStatement *CBEOpDummy::buildClientUnmarshal()

{ 
  /* Copy the elements out of the marshal buffer and into the buffers supplied
     by the client program. Note that the marshal buffer is temporary; you
     should not return any references to it */

  return NULL; 
}

void CBEOpDummy::buildServerDeclarations(CBEVarSource *varSource)

{
  /* Use the varSource to allocate any temporary variables you will need
     on the server side */
}

CASTStatement *CBEOpDummy::buildServerUnmarshal()

{ 
  /* In contrast to the client side, the server marshal buffer remains
     allocated while the operation is executing; it is freed after the
     reply is sent. Thus, it is possible to use references in order
     to avoid superfluous copies */

  return NULL; 
}

CASTExpression *CBEOpDummy::buildServerArg(CBEParameter *param)

{ 
  /* An operation may be responsible for one or more parameters of the
     function it is associated with; e.g. a string copy operation may
     be responsible for the string pointer and its length. In this case,
     'responsible' means that the operation will supply an argument
     for the service function on the server side (e.g. a pointer to
     some part of the marshal buffer). If 'param' is one of your
     parameters, return the corresponding server argument, otherwise
     return NULL. */

  return NULL; 
}

CASTStatement *CBEOpDummy::buildServerMarshal()

{ 
  /* Copy the elements back to the server marhsal buffer. If you have
     used references in buildServerUnmarshal(), you can omit this step */

  return NULL; 
}
