#include "ops.h"
#include "cast.h"

CASTStatement *CBEAliasType::buildDefinition()

{
  if (aoi->flags&FLAG_DONTDEFINE)
    return NULL;

  int *dimension = (int*)&aoi->dimension;

  /* build array dimensions */

  CASTExpression *arrayDims = NULL;
  for (int i=0;i<aoi->numBrackets;i++)
    addTo(arrayDims, new CASTIntegerConstant(dimension[i]));
 
  /* build declaration and add typedef storage class */

  CASTIdentifier *name = getType(aoi)->buildIdentifier();
  CASTDeclarator *decl = new CASTDeclarator(name, NULL, arrayDims);
  CASTDeclaration *declar = getType(aoi->ref)->buildDeclaration(decl);
  declar->addSpecifier(new CASTStorageClassSpecifier("typedef"));
 
  CASTStatement *subtree = NULL;
  addTo(subtree, buildConditionalDefinition(new CASTDeclarationStatement(declar)));
  addTo(subtree, new CASTSpacer());
  
  return subtree;
}

int CBEAliasType::getFlatSize()

{
  int size = getType(aoi->ref)->getFlatSize();
  
  for (int i=0;i<aoi->numBrackets;i++)
    size *= aoi->dimension[i];
    
  return size;
}

CASTDeclaration *CBEAliasType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound)

{
  if (aoi->name)
    return new CASTDeclaration(new CASTTypeSpecifier(buildIdentifier()), decl, compound);
  else if (aoi->numBrackets != 0)
    semanticError(aoi->context, "alias types with dimensions need typedefs");

  return getType(aoi->ref)->buildDeclaration(decl, compound);
}

CASTDeclaration *CBEAliasType::buildMessageDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound)

{
  CBEType *type = getType(aoi->ref);
  if (type->needsSpecialHandling() && aoi->numBrackets == 0)
    return type->buildMessageDeclaration(decl, compound);
  else
    return buildDeclaration(decl, compound);
}

CBEMarshalOp *CBEAliasType::buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags)

{
  if (aoi->numBrackets == 0)
    return getType(aoi->ref)->buildMarshalOps(connection, channels, rvalue, name, originalType, param, flags);

  if (!getType(aoi->ref)->needsSpecialHandling())
    return new CBEOpSimpleCopy(connection, channels, rvalue, originalType, getFlatSize(), name, param, flags);

  warning("Not implemented: Marshalling complex alias types");
  return new CBEOpDummy(param); 
}

int CBEAliasType::getArgIndirLevel(CBEDeclType declType)

{ 
  int result = 0;

  if (aoi->numBrackets == 0)
    result += getType(aoi->ref)->getArgIndirLevel(declType);
    
  return result;
}

void CBEAliasType::buildIndexExpression(CASTExpression **pathToItem, CASTIdentifier *loopVar[])

{
  for (int i=aoi->numBrackets-1;i>=0;i--)
    {
      *pathToItem = new CASTIndexOp(
        *pathToItem,
        loopVar[i]);
    }
}

CASTStatement *CBEAliasType::buildArrayScan(CASTStatement *preStatement, CASTStatement *innerStatement, CASTStatement *postStatement, CASTIdentifier *loopVar[])

{
  CASTStatement *result = innerStatement;
      
  for (int i=aoi->numBrackets-1;i>=0;i--)
    {
      CASTStatement *compoundStatement = NULL;
      if (preStatement || postStatement)
        {
          if (preStatement)
            addTo(compoundStatement, preStatement->clone());
          addTo(compoundStatement, result);
          if (postStatement)
            addTo(compoundStatement, postStatement->clone());
          compoundStatement = new CASTCompoundStatement(compoundStatement);
        } else compoundStatement = result;
    
      result = new CASTForStatement(
        new CASTExpressionStatement(
          new CASTBinaryOp("=",
            loopVar[i]->clone(),
            new CASTIntegerConstant(0))),
        new CASTBinaryOp("<",
          loopVar[i]->clone(),
          new CASTIntegerConstant(aoi->dimension[i])),
        new CASTPostfixOp("++",
          loopVar[i]->clone()),
        compoundStatement
      );
    } 
        
  return result;    
}

CASTStatement *CBEAliasType::buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param)

{
  if (aoi->numBrackets==0)
    return getType(aoi->ref)->buildTestClientInit(globalPath, localPath, varSource, type, param);

  CASTIdentifier *loopVar[MAX_DIMENSION];
  for (int i=0;i<aoi->numBrackets;i++)
    loopVar[i] = varSource->requestVariable(true, msFactory->getMachineWordType());

  buildIndexExpression(&globalPath, loopVar);
  buildIndexExpression(&localPath, loopVar);

  CASTStatement *elementAction = getType(aoi->ref)->buildTestClientInit(globalPath, localPath, varSource, type, param);
  if (!elementAction)
    return NULL;

  return buildArrayScan(
    NULL,
    elementAction,
    NULL, 
    loopVar
  );
}

CASTStatement *CBEAliasType::buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  if (aoi->numBrackets==0)
    return getType(aoi->ref)->buildTestClientCheck(globalPath, localPath, varSource, type);

  CASTIdentifier *loopVar[MAX_DIMENSION];
  for (int i=0;i<aoi->numBrackets;i++)
    loopVar[i] = varSource->requestVariable(true, msFactory->getMachineWordType());

  buildIndexExpression(&globalPath, loopVar);
  buildIndexExpression(&localPath, loopVar);

  return buildArrayScan(
    NULL,
    getType(aoi->ref)->buildTestClientCheck(globalPath, localPath, varSource, type),
    NULL,
    loopVar
  );
}

CASTStatement *CBEAliasType::buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  return getType(aoi->ref)->buildTestClientPost(globalPath, localPath, varSource, type);
}

CASTStatement *CBEAliasType::buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  return getType(aoi->ref)->buildTestClientCleanup(globalPath, localPath, varSource, type);
}

CASTStatement *CBEAliasType::buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable)

{
  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  if (aoi->numBrackets==0)
    return getType(aoi->ref)->buildTestServerInit(globalPath, localPath, varSource, type, param, bufferAvailable);
        
  CASTIdentifier *loopVar[MAX_DIMENSION];
  for (int i=0;i<aoi->numBrackets;i++)
    loopVar[i] = varSource->requestVariable(true, msFactory->getMachineWordType());

  buildIndexExpression(&globalPath, loopVar);
  buildIndexExpression(&localPath, loopVar);

  return buildArrayScan(
    NULL,
    getType(aoi->ref)->buildTestServerInit(globalPath, localPath, varSource, type, param, bufferAvailable),
    NULL,
    loopVar
  );
}

CASTStatement *CBEAliasType::buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if ((type!=IN) && (type!=INOUT))
    return NULL;

  if (aoi->numBrackets==0)
    return getType(aoi->ref)->buildTestServerCheck(globalPath, localPath, varSource, type);

  CASTIdentifier *loopVar[MAX_DIMENSION];
  for (int i=0;i<aoi->numBrackets;i++)
    loopVar[i] = varSource->requestVariable(true, msFactory->getMachineWordType());

  buildIndexExpression(&globalPath, loopVar);
  buildIndexExpression(&localPath, loopVar);

  return buildArrayScan(
    NULL,
    getType(aoi->ref)->buildTestServerCheck(globalPath, localPath, varSource, type),
    NULL,
    loopVar
  );
}

CASTStatement *CBEAliasType::buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  if (type!=IN)
    return NULL;

  if (aoi->numBrackets==0)
    return getType(aoi->ref)->buildTestServerRecheck(globalPath, localPath, varSource, type);

  CASTIdentifier *loopVar[MAX_DIMENSION];
  for (int i=0;i<aoi->numBrackets;i++)
    loopVar[i] = varSource->requestVariable(true, msFactory->getMachineWordType());

  buildIndexExpression(&globalPath, loopVar);
  buildIndexExpression(&localPath, loopVar);

  return buildArrayScan(
    NULL,
    getType(aoi->ref)->buildTestServerRecheck(globalPath, localPath, varSource, type),
    NULL,
    loopVar
  );
}

CASTStatement *CBEAliasType::buildAssignment(CASTExpression *dest, CASTExpression *src)

{
  /* Special handling is required for arrays only */

  if (!aoi->numBrackets)
    return CBEType::buildAssignment(dest, src);
   
  /* If the array is 'big enough', we use memcpy */
   
  if ((getFlatSize()>(5*globals.word_size)) && (getAlignment()>=globals.word_size))
    {
      return new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("idl4_memcpy"),
          knitExprList(
            dest,
            src,
            new CASTIntegerConstant((getFlatSize()+globals.word_size-1)/globals.word_size)))
      );
    }  

  /* For small arrays, we use separate statements for each element.
     Thus, the compiler can apply its own optimizations (e.g. move the
     assignments around) */

  CASTStatement *result = NULL;

  int dim[MAX_DIMENSION], pos;
  for (int i=0;i<aoi->numBrackets;i++)
    dim[i]=0;
    
  do {
       CASTExpression *destElem = dest->clone();
       CASTExpression *srcElem = src->clone();
       
       for (int i=0;i<aoi->numBrackets;i++)
         {
           destElem = new CASTIndexOp(destElem, new CASTIntegerConstant(dim[i]));
           srcElem = new CASTIndexOp(srcElem, new CASTIntegerConstant(dim[i]));
         }
       
       addTo(result, getType(aoi->ref)->buildAssignment(destElem, srcElem));
       
       pos = aoi->numBrackets - 1;
       do {
            dim[pos]++;
            if (dim[pos] >= aoi->dimension[pos])
              dim[pos--] = 0;
              else break;
          } while (pos>=0);
     } while (pos>=0);    

  return result;
}

CASTExpression *CBEAliasType::buildDefaultValue()

{
  if (!aoi->numBrackets)
    return getType(aoi->ref)->buildDefaultValue();

  return new CASTArrayInitializer(getType(aoi->ref)->buildDefaultValue());
}

int CBEAliasType::getAlignment()

{
  return getType(aoi->ref)->getAlignment();
}

bool CBEAliasType::needsSpecialHandling()

{
  return getType(aoi->ref)->needsSpecialHandling();
}

bool CBEAliasType::needsDirectCopy()

{
  return getType(aoi->ref)->needsDirectCopy();
}

bool CBEAliasType::involvesMapping()

{
  return getType(aoi->ref)->involvesMapping();
}

CASTStatement *CBEAliasType::buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value)

{
  if (aoi->numBrackets==0)
    return getType(aoi->ref)->buildTestDisplayStmt(globalPath, localPath, varSource, type, value);

  CASTIdentifier *loopVar[MAX_DIMENSION];
  for (int i=0;i<aoi->numBrackets;i++)
    loopVar[i] = varSource->requestVariable(true, msFactory->getMachineWordType());

  buildIndexExpression(&globalPath, loopVar);
  buildIndexExpression(&localPath, loopVar);
  buildIndexExpression(&value, loopVar);

  CASTStatement *elementAction = getType(aoi->ref)->buildTestDisplayStmt(globalPath, localPath, varSource, type, value);
  if (!elementAction)
    return NULL;

  return buildArrayScan(
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("printf"),
        new CASTStringConstant(false, "{ "))
    ),
    elementAction,
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("printf"),
        new CASTStringConstant(false, "} "))
    ),
    loopVar
  );
}

CBEParameter *CBEAliasType::getFirstMemberWithProperty(const char *property)

{
  return getType(aoi->ref)->getFirstMemberWithProperty(property);
}

bool CBEAliasType::isImplicitPointer()

{
  return (aoi->numBrackets > 0);
}

bool CBEAliasType::isAllowedParamType()

{
  return getType(aoi->ref)->isAllowedParamType();
}
