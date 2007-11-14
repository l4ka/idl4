#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

CBEMarshalOp *CBEFloatType::buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags)

{
  CBEMarshalOp *result;

  dprintln("Marshalling %s (%s)", name, aoi->name);
  indent(+1);

  result = new CBEOpSimpleCopy(connection, channels, rvalue, originalType, aoi->sizeInBytes, name, param, flags);
  
  indent(-1);
  
  return result;
}

CASTDeclaration *CBEFloatType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound)

{
  if (globals.flags&FLAG_CTYPES)
    {
      int sizeInBytes = aoi->sizeInBytes;
      CASTDeclarationSpecifier *specifiers = NULL;
  
      switch (sizeInBytes)
        {
          case 4 : addTo(specifiers, new CASTTypeSpecifier("float"));
                   break;           
          case 8 : addTo(specifiers, new CASTTypeSpecifier("double"));
                   break;           
          case 12: addTo(specifiers, new CASTTypeSpecifier("long"));
                   addTo(specifiers, new CASTTypeSpecifier("double"));
                   break;           
          default: assert(false);
        }
    
      return new CASTDeclaration(specifiers, decl, compound);
    }
  
  return new CASTDeclaration(new CASTTypeSpecifier(aoi->alias), decl, compound);
}

int CBEFloatType::getArgIndirLevel(CBEDeclType declType)

{ 
  if ((declType==INOUT) || (declType==OUT))
    return 1;

  return 0;    
}

CASTStatement *CBEFloatType::buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param)

{
  if ((type!=IN) && (type!=INOUT))
    return NULL;

  CASTExpression *randomValue = new CASTFunctionOp(
    new CASTIdentifier("random")
  );
  
  if (aoi->sizeInBytes<4)
    randomValue = new CASTBrackets(
      new CASTBinaryOp("+",
        new CASTIntegerConstant(1),
        new CASTBrackets(
          new CASTBinaryOp("&", 
            randomValue,
            new CASTHexConstant((1<<(aoi->sizeInBytes*8))-2))))
    );
  
  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "svr_in"),
      new CASTBinaryOp("=",
        knitPathInstance(globalPath, "client"),
        randomValue))
  );
}

CASTStatement *CBEFloatType::buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  char info[100];

  enableStringMode(info, sizeof(info));
  localPath->write();
  disableStringMode();

  return new CASTIfStatement(
    new CASTBinaryOp("!=",
      knitPathInstance(globalPath, "client"),
      knitPathInstance(globalPath, "svr_out")),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(new CASTStringConstant(false, mprintf("Out transfer fails for %s: %%d!=%%d", info)),
                     new CASTUnaryOp("(int)", knitPathInstance(globalPath, "client")->clone()),
                     new CASTUnaryOp("(int)", knitPathInstance(globalPath, "svr_out")->clone()))))
  );
}

CASTStatement *CBEFloatType::buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  return NULL;
}

CASTStatement *CBEFloatType::buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  return NULL;
}

CASTStatement *CBEFloatType::buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable)

{
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  CASTExpression *randomValue = new CASTFunctionOp(
    new CASTIdentifier("random")
  );

  if (aoi->sizeInBytes<4)
    randomValue = new CASTBrackets(
      new CASTBinaryOp("+",
        new CASTIntegerConstant(1),
        new CASTBrackets(
          new CASTBinaryOp("&", 
            randomValue,
            new CASTHexConstant((1<<(aoi->sizeInBytes*8))-2))))
    );
  
  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "svr_out"),
      new CASTBinaryOp("=",
        localPath,
        randomValue))
  );
}

CASTStatement *CBEFloatType::buildTestServerCheckGeneric(CASTExpression *globalPath, CASTExpression *localPath, const char *text)

{
  char info[100];

  enableStringMode(info, sizeof(info));
  localPath->write();
  disableStringMode();

  return new CASTIfStatement(
    new CASTBinaryOp("!=",
      localPath->clone(),
      knitPathInstance(globalPath, "svr_in")),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(new CASTStringConstant(false, mprintf("%s for %s: %%d!=%%d", text, info)),
                     localPath->clone(),
                     knitPathInstance(globalPath, "svr_in"))))
  );
}

CASTStatement *CBEFloatType::buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if ((type!=IN) && (type!=INOUT))
    return NULL;

  return buildTestServerCheckGeneric(globalPath, localPath, "In transfer fails");
}

CASTStatement *CBEFloatType::buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if (type!=IN)
    return NULL;

  return buildTestServerCheckGeneric(globalPath, localPath, "Output value collision detected");
}

CASTExpression *CBEFloatType::buildDefaultValue()

{
  return new CASTFloatConstant(0);
}

CASTExpression *CBEFloatType::buildBufferAllocation(CASTExpression *elements)

{
  const char *typeName;
  
  switch (aoi->sizeInBytes)
    {
      case  4 : typeName = "float"; break;
      case  8 : typeName = "double"; break;
      case 12 : typeName = "long_double"; break;
      default : panic("Unknown floating point type: %s", aoi->name); break;
    }
      
  return new CASTFunctionOp(
    new CASTIdentifier(mprintf("CORBA_%s_alloc", typeName)), 
    elements
  );
}

CASTStatement *CBEFloatType::buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value)

{
  const char *formatString = (aoi->sizeInBytes==4) ? "%f " : "%g ";
 
  return new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("printf"),
      knitExprList(
        new CASTStringConstant(false, formatString),
        value))
  );
}
