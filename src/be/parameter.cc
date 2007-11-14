#include "be.h"
#include "ops.h"

CBEParameter::CBEParameter(CAoiParameter *aoi) : CBEBase(aoi)

{
  this->aoi = aoi;
  this->priority = 0;
  this->marshalled = false;

  if (aoi->flags&FLAG_IN)
    {
      if (aoi->flags&FLAG_OUT)
        type = INOUT;
        else type = IN;
    } else {
             if (aoi->flags & FLAG_OUT) 
               type = OUT;
               else type = ELEMENT;
           }
}

CASTDeclaration *CBEParameter::buildDeclaration()

{
  CASTPointer *pointers = (aoi->refLevel>0) ? new CASTPointer(aoi->refLevel) : NULL;
  CASTExpression *arrayDims = NULL;
  
  for (int i=0;i<aoi->numBrackets;i++)
    addTo(arrayDims, new CASTIntegerConstant(aoi->dimension[i]));
 
  CASTDeclarator *decl =new CASTDeclarator(new CASTIdentifier(aoi->name), pointers, arrayDims);
  decl->addIndir(getType(aoi->type)->getArgIndirLevel(type));
  if (aoi->bits)
    decl->setBitSize(new CASTIntegerConstant(aoi->bits));
 
  CASTDeclaration *dclr = getType(aoi->type)->buildDeclaration(decl);
  if (type==IN && !getType(aoi->type)->isImplicitPointer())
    dclr->addSpecifier(new CASTConstOrVolatileSpecifier("const"));
    
  return dclr;
}

CBEParameter *CBEParameter::getPropertyParameter(const char *propName)

{
  forAll(aoi->properties,
    {
      CAoiProperty *prop = (CAoiProperty*)item;
      if (!strcmp(prop->name, propName))
        {
          assert(aoi->parent);
          assert(prop->refValue);
          CAoiParameter *sizeIs = (CAoiParameter*)(((CAoiOperation*)aoi->parent)->parameters)->getByName(prop->refValue);
          if (sizeIs)
            return getParameter(sizeIs);
        }
    }
  );        
  return NULL;
}

CASTIdentifier *CBEParameter::buildIdentifier()

{
  return new CASTIdentifier(aoi->name);
}

CASTStatement *CBEParameter::buildTestDisplayStmt(CASTExpression *globalPath, CBEVarSource *varSource, CASTExpression *value)

{
  const char *dirStr = getType(aoi->type)->getDirectionString(type);
  CASTStatement *result = NULL;

  char nameStr[100];
  enableStringMode(nameStr, sizeof(nameStr));
  getType(aoi->type)->buildDeclaration(new CASTDeclarator(buildIdentifier()))->write();
  disableStringMode();
      
  addTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("printf"),
      knitExprList(
        new CASTStringConstant(false, mprintf("%s %s = ", dirStr, nameStr)))))
  );

  addTo(result, getType(aoi->type)->buildTestDisplayStmt(
    new CASTBinaryOp(".", globalPath, buildIdentifier()),
    buildIdentifier(),
    varSource, type, value)
  );
  
  addTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("printf"),
      new CASTStringConstant(false, "\\n")))
  );
  
  return result;
}

CASTStatement *CBEParameter::buildTestClientInit(CASTExpression *globalPath, CBEVarSource *varSource)

{
  if (aoi->refLevel || aoi->numBrackets)
    warning("Not implemented: Test suite for complex alias types");

  return getType(aoi->type)->buildTestClientInit(
    new CASTBinaryOp(".", globalPath, buildIdentifier()),
    buildIdentifier(),
    varSource, type, this);
}

CASTExpression *CBEParameter::buildTestClientArg(CASTExpression *globalPath)

{
  globalPath = new CASTBinaryOp(".",
    globalPath,
    new CASTIdentifier(aoi->name)
  );
  
  for (int i=0;i<getType(aoi->type)->getArgIndirLevel(type);i++)
    globalPath = new CASTUnaryOp("&", globalPath);
  
  return globalPath;
}

CASTStatement *CBEParameter::buildTestClientCheck(CASTExpression *globalPath, CBEVarSource *varSource)

{
  return getType(aoi->type)->buildTestClientCheck(
    new CASTBinaryOp(".", globalPath, buildIdentifier()),
    buildIdentifier(),
    varSource, type);
}

CASTStatement *CBEParameter::buildTestClientPost(CASTExpression *globalPath, CBEVarSource *varSource)

{
  return getType(aoi->type)->buildTestClientPost(
    new CASTBinaryOp(".", globalPath, buildIdentifier()),
    buildIdentifier(),
    varSource, type);
}

CASTStatement *CBEParameter::buildTestClientCleanup(CASTExpression *globalPath, CBEVarSource *varSource)

{
  return getType(aoi->type)->buildTestClientCleanup(
    new CASTBinaryOp(".", globalPath, buildIdentifier()),
    buildIdentifier(),
    varSource, type);
}

CASTExpression *CBEParameter::buildRvalue(CASTExpression *expr)

{
  for (int i=0;i<getType(aoi->type)->getArgIndirLevel(type);i++)
    expr = new CASTUnaryOp("*", expr);
    
  return expr;
}

CASTStatement *CBEParameter::buildTestServerInit(CASTExpression *globalPath, CBEVarSource *varSource, bool bufferAvailable)

{
  return getType(aoi->type)->buildTestServerInit(
    new CASTBinaryOp(".", globalPath, buildIdentifier()),
    buildRvalue(buildIdentifier()),
    varSource, type, this, bufferAvailable);
}

CASTStatement *CBEParameter::buildTestServerCheck(CASTExpression *globalPath, CBEVarSource *varSource)

{
  return getType(aoi->type)->buildTestServerCheck(
    new CASTBinaryOp(".", globalPath, buildIdentifier()),
    buildRvalue(buildIdentifier()),
    varSource, type);
}

CASTStatement *CBEParameter::buildTestServerRecheck(CASTExpression *globalPath, CBEVarSource *varSource)

{
  return getType(aoi->type)->buildTestServerRecheck(
    new CASTBinaryOp(".", globalPath, buildIdentifier()),
    buildRvalue(buildIdentifier()),
    varSource, type);
}

CBEMarshalOp *CBEParameter::buildMarshalOps(CMSConnection *connection, int flags)

{
  int channels = 0;
      
  if (this->marshalled)
    return NULL;
      
  this->marshalled = true;
      
  if (aoi->flags&FLAG_IN)
    channels |= CHANNEL_MASK(CHANNEL_IN);
  if (aoi->flags&FLAG_OUT)
    channels |= CHANNEL_MASK(CHANNEL_OUT);
      
  if (!(aoi->flags&FLAG_NOXFER))
    return getType(aoi->type)->buildMarshalOps(
      connection, 
      channels, 
      buildRvalue(buildIdentifier()),
      aoi->name, 
      getType(aoi->type),
      this,
      flags
    );
    else return new CBEOpAllocOnServer(connection, getType(aoi->type), this, flags);
}

void CBEParameter::registerDependencies(CBEDependencyList *dependencyList)

{
  CAoiProperty *prop;
  
  dependencyList->registerParam(this);
  
  prop = aoi->getProperty("length_is");
  if (prop && (prop->refValue))
    {
      CBEParameter *lengthIsParam = getPropertyParameter("length_is");
      if (lengthIsParam)
        {
          if (getType(lengthIsParam->aoi->type)->isScalar())
            {
              int elemSize = getType(lengthIsParam->aoi->type)->getFlatSize();
              assert(elemSize!=UNKNOWN);
              dependencyList->registerDependency(this, lengthIsParam);
              lengthIsParam->aoi->properties->add(
                aoiFactory->buildProperty("max_value", 
                  aoiFactory->buildConstInt(SERVER_BUFFER_SIZE / elemSize, aoi->context),
                  aoi->context)
              );
            } else semanticError(prop->context, "length must be scalar");
        } else semanticError(prop->context, "undefined reference: length_is");
    }
}

CBEParameter *CBEParameter::getFirstMemberWithProperty(const char *property)

{
  if (aoi->getProperty(property))
    return this;

  return getType(aoi->type)->getFirstMemberWithProperty(property);
}
