#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

CBEMarshalOp *CBEWordType::buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags)

{
  bool isSigned = aoi->isSigned;

  CBEMarshalOp *result;

  dprintln("Marshalling %s (%s)", name, aoi->name);
  indent(+1);

  if (globals.compat_mode && isSigned)
    result = new CBEOpCompatCopy(connection, channels, rvalue, originalType, getFlatSize(), name, param, flags);
  else
    result = new CBEOpSimpleCopy(connection, channels, rvalue, originalType, getFlatSize(), name, param, flags);
  
  indent(-1);
  
  return result;
}

CASTDeclaration *CBEWordType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound)

{
  bool isSigned = aoi->isSigned;

  return new CASTDeclaration(new CASTTypeSpecifier(isSigned ? "idl4_signed_word_t" : "idl4_word_t"),
                             decl, compound);
}

CASTDeclaration *CBEWordType::buildMessageDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound)

{
  bool isSigned = aoi->isSigned;

  if (globals.compat_mode && isSigned)
    return new CASTDeclaration(new CASTTypeSpecifier("idl4_msg_signed_word_t"), decl, compound);
  else
    return buildDeclaration(decl, compound);
}

CAoiWordType *aoiMachineWordType, *aoiSignedMachineWordType;

CAoiWordType *createAoiMachineWordType(CAoiScope *parentScope, CAoiContext *context)

{
  if (!aoiMachineWordType)
    aoiMachineWordType = new CAoiWordType("word", false, parentScope, context);
  return aoiMachineWordType;
}

CAoiWordType *createAoiSignedMachineWordType(CAoiScope *parentScope, CAoiContext *context)

{
  if (!aoiSignedMachineWordType)
    aoiSignedMachineWordType = new CAoiWordType("signed_word", true, parentScope, context);
  return aoiSignedMachineWordType;
}

CBEWordType *machineWordType;

CBEWordType *CMSFactory::getMachineWordType()

{
  if (!machineWordType)
    machineWordType = new CBEWordType(aoiMachineWordType);
  return machineWordType;
}
