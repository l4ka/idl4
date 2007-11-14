#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

CBEMarshalOp *CBEIntegerType::buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags)

{
  CBEMarshalOp *result;

  dprintln("Marshalling %s (%s)", name, aoi->name);
  indent(+1);

  result = new CBEOpSimpleCopy(connection, channels, rvalue, originalType, aoi->sizeInBytes, name, param, flags);
  
  indent(-1);
  
  return result;
}

CASTDeclaration *CBEIntegerType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound)

{
  if (globals.flags&FLAG_CTYPES)
    {
      bool isSigned = aoi->isSigned;
      int sizeInBytes = aoi->sizeInBytes;
      CASTDeclarationSpecifier *specifiers = NULL;

      if (!isSigned)
        addTo(specifiers, new CASTTypeSpecifier("unsigned"));
    
      switch (sizeInBytes)
        {
          case 0 : addTo(specifiers, new CASTTypeSpecifier("void"));
                   break;           
          case 1 : addTo(specifiers, new CASTTypeSpecifier("char"));
                   break;           
          case 2 : addTo(specifiers, new CASTTypeSpecifier("short"));
                   break;           
          case 4 : addTo(specifiers, new CASTTypeSpecifier("int"));
                   break;           
          case 8 : addTo(specifiers, new CASTTypeSpecifier("long"));
                   if (globals.word_size<8)
                     addTo(specifiers, new CASTTypeSpecifier("long"));      
                   break;           
          default: assert(false);
        }
    
      return new CASTDeclaration(specifiers, decl, compound);
    }
    
  return new CASTDeclaration(new CASTTypeSpecifier(aoi->alias), decl, compound);
}

int CBEIntegerType::getArgIndirLevel(CBEDeclType declType)

{ 
  if ((declType==INOUT) || (declType==OUT))
    return 1;
    
  return 0;
}

CASTStatement *CBEIntegerType::buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param)

{
  CASTStatement *result = NULL;

  if ((type!=IN) && (type!=INOUT))
    return NULL;

  CASTExpression *randomValue = new CASTFunctionOp(
    new CASTIdentifier("random")
  );
  
  if (aoi->sizeInBytes<globals.word_size)
    randomValue = new CASTBrackets(
      new CASTBinaryOp("+",
        new CASTIntegerConstant(1),
        new CASTBrackets(
          new CASTBinaryOp("&", 
            randomValue,
            new CASTHexConstant((1<<(aoi->sizeInBytes*8))-2))))
    );
  
  CAoiProperty *maxValueProp = (param!=NULL) ? param->aoi->getProperty("max_value") : NULL;
  if (maxValueProp)
    {
      assert(maxValueProp->constValue);
      
      int maxValue =((CAoiConstInt*)maxValueProp->constValue)->value;
      randomValue = new CASTBinaryOp("+",
        new CASTBrackets(
          new CASTBinaryOp("%",
            randomValue,
            new CASTIntegerConstant(maxValue))),
        new CASTIntegerConstant(1)
      );
    }
  
  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "svr_in"),
      new CASTBinaryOp("=",
        knitPathInstance(globalPath, "client"),
        randomValue))
  );
}

CASTStatement *CBEIntegerType::buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

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

CASTStatement *CBEIntegerType::buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  return NULL;
}

CASTStatement *CBEIntegerType::buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  return NULL;
}

CASTStatement *CBEIntegerType::buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable)

{
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  CASTExpression *randomValue = new CASTFunctionOp(
    new CASTIdentifier("random")
  );

  if (aoi->sizeInBytes<globals.word_size)
    randomValue = new CASTBrackets(
      new CASTBinaryOp("+",
        new CASTIntegerConstant(1),
        new CASTBrackets(
          new CASTBinaryOp("&", 
            randomValue,
            new CASTHexConstant((1<<(aoi->sizeInBytes*8))-2))))
    );

  CAoiProperty *maxValueProp = (param!=NULL) ? param->aoi->getProperty("max_value") : NULL;
  if (maxValueProp)
    {
      assert(maxValueProp->constValue);
      
      int maxValue =((CAoiConstInt*)maxValueProp->constValue)->value;
      randomValue = new CASTBinaryOp("+",
        new CASTBrackets(
          new CASTBinaryOp("%",
            randomValue,
            new CASTIntegerConstant(maxValue))),
        new CASTIntegerConstant(1)
      );
    }
  
  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      knitPathInstance(globalPath, "svr_out"),
      new CASTBinaryOp("=",
        localPath,
        randomValue))
  );
}

CASTStatement *CBEIntegerType::buildTestServerCheckGeneric(CASTExpression *globalPath, CASTExpression *localPath, const char *text)

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

CASTStatement *CBEIntegerType::buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if ((type!=IN) && (type!=INOUT))
    return NULL;

  return buildTestServerCheckGeneric(globalPath, localPath, "In transfer fails");
}

CASTStatement *CBEIntegerType::buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if (type!=IN)
    return NULL;

  return buildTestServerCheckGeneric(globalPath, localPath, "Output value collision detected");
}

CASTExpression *CBEIntegerType::buildBufferAllocation(CASTExpression *elements)

{
  char *ident;
  
  if (aoi->sizeInBytes==1 || aoi->sizeInBytes==2)
    ident = mprintf("CORBA_%s_alloc", (aoi->sizeInBytes==2) ? "wstring" : "string");
    else ident = mprintf("%s_alloc", aoi->alias);
    
  return new CASTFunctionOp(
    new CASTIdentifier(ident),
    new CASTUnaryOp("(unsigned)", 
      elements)
  );
}

CASTExpression *CBEIntegerType::buildDefaultValue()

{
  return new CASTIntegerConstant(0);
}

int CBEIntegerType::getAlignment()

{
  if (aoi->sizeInBytes>globals.word_size)
    return globals.word_size;
    
  if (aoi->sizeInBytes<1)
    return 1;
    
  return aoi->sizeInBytes;
}

CASTStatement *CBEIntegerType::buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value)

{
  const char *formatString = (aoi->sizeInBytes==1) ? "'%c' " : "%d ";
 
  return new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("printf"),
      knitExprList(
        new CASTStringConstant(false, formatString),
        value))
  );
}
