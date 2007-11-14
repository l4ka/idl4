#include "ops.h"

CBEMarshalOp *CBEStructType::buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags)

{
  forAll(aoi->members, 
    {
      CBEType *memberType = getType(((CAoiParameter*)item)->type);

      if (memberType->needsSpecialHandling())
        panic("Cannot marshal struct %s: Member %s needs special handling", aoi->name, item->name);

      if (!memberType->needsDirectCopy())
        panic("Cannot marshal struct %s: Member %s does not need to be copied", aoi->name, item->name);
    }
  );

  return new CBEOpSimpleCopy(connection, channels, rvalue, originalType, getFlatSize(), name, param, flags);
}

CASTDeclarationSpecifier *CBEStructType::buildSpecifier()

{
  CASTDeclarationStatement *members = NULL;
  forAll(aoi->members, addTo(members, new CASTDeclarationStatement(getParameter(item)->buildDeclaration())));
  return new CASTAggregateSpecifier("struct", buildIdentifier(), members); 
}

CASTDeclaration *CBEStructType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound)

{ 
  if (aoi->name) 
    {
      CASTDeclarationSpecifier *spec = NULL;
      addTo(spec, new CASTStorageClassSpecifier("struct"));
      addTo(spec, new CASTTypeSpecifier(buildIdentifier()));
      return new CASTDeclaration(spec, decl, compound);
    } else return new CASTDeclaration(buildSpecifier(), decl, compound); 
}

CASTStatement *CBEStructType::buildDefinition()

{
  if (aoi->flags & FLAG_DONTDEFINE)
    return NULL;

  CASTStatement *result = NULL;
  
  addTo(result, buildConditionalDefinition(
    new CASTDeclarationStatement(
      new CASTDeclaration(buildSpecifier(), ANONYMOUS)))
  );    
  
  addTo(result, new CASTSpacer());
  
  return result;
}

int CBEStructType::getArgIndirLevel(CBEDeclType declType)

{ 
  if ((declType==IN) || (declType==INOUT) || (declType==OUT))
    return 1;
    
  return 0;
}

void CBEStructType::prepareIteration(CAoiParameter *param, CASTExpression **globalPath, CASTExpression **localPath, CASTIdentifier *loopVar[], CBEVarSource *varSource)

{
  for (int i=0;i<param->numBrackets;i++)
    {
      loopVar[i] = varSource->requestVariable(true, msFactory->getMachineWordType());
      *globalPath = new CASTIndexOp(*globalPath, loopVar[i]->clone());
      *localPath = new CASTIndexOp(*localPath, loopVar[i]->clone());
    }
}

CASTStatement *CBEStructType::iterate(CAoiParameter *param, CASTStatement *memberTest, CASTIdentifier *loopVar[])

{
  if (param->numBrackets>0)
    memberTest = new CASTCompoundStatement(memberTest);
      
  for (int i=param->numBrackets-1;i>=0;i--)
    memberTest = new CASTForStatement(
      new CASTExpressionStatement(new CASTBinaryOp("=", loopVar[i]->clone(), new CASTIntegerConstant(0))),
      new CASTBinaryOp("<", loopVar[i]->clone(), new CASTIntegerConstant(param->dimension[i])),
      new CASTPostfixOp("++", loopVar[i]->clone()),
      memberTest);
      
  return memberTest;
}

CASTStatement *CBEStructType::buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param)

{
  CASTStatement *result = NULL;

  if ((type!=IN) && (type!=INOUT))
    return NULL;

  addTo(result, new CASTIfStatement(
    new CASTBinaryOp("!=",
      new CASTFunctionOp(
        new CASTIdentifier("sizeof"),
        knitPathInstance(globalPath, "client")),
      new CASTIntegerConstant(getFlatSize())),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(new CASTStringConstant(false, mprintf("Size of %s is %%d (assumed %d)", (aoi->name) ? aoi->name : "anonymous struct", getFlatSize())),
                     new CASTFunctionOp(new CASTIdentifier("sizeof"), knitPathInstance(globalPath, "client"))))))
  );

  forAll(aoi->members, 
    {
      CAoiParameter *param = (CAoiParameter*)item;
      CBEType *memberType = getType(param->type);
      CASTExpression *globalMemberPath = new CASTBinaryOp(".", globalPath, new CASTIdentifier(param->name));
      CASTExpression *localMemberPath = new CASTBinaryOp(".", localPath, new CASTIdentifier(param->name));
      CASTIdentifier *loopVar[MAX_DIMENSION];
      
      prepareIteration(param, &globalMemberPath, &localMemberPath, loopVar, varSource);
      addTo(result, iterate(param,
        memberType->buildTestClientInit(
          globalMemberPath,
          localMemberPath,
          varSource, type, getParameter(param)),
        loopVar)
      );
    }
  );
  
  return result;
}

CASTStatement *CBEStructType::buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{ 
  return NULL;
}

CASTStatement *CBEStructType::buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{ 
  return NULL;
}

CASTStatement *CBEStructType::buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  CASTStatement *result = NULL;

  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  forAll(aoi->members, 
    {
      CAoiParameter *param = (CAoiParameter*)item;
      CBEType *memberType = getType(param->type);
      CASTExpression *globalMemberPath = new CASTBinaryOp(".", globalPath, new CASTIdentifier(param->name));
      CASTExpression *localMemberPath = new CASTBinaryOp(".", localPath, new CASTIdentifier(param->name));
      CASTIdentifier *loopVar[MAX_DIMENSION];

      prepareIteration(param, &globalMemberPath, &localMemberPath, loopVar, varSource);
      addTo(result, iterate(param,
        memberType->buildTestClientCheck(
          globalMemberPath,
          localMemberPath,
          varSource, type),
        loopVar)
      );
    }
  );
  
  return result;
}

CASTStatement *CBEStructType::buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable)

{
  CASTStatement *result = NULL;

  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  forAll(aoi->members, 
    {
      CAoiParameter *param = (CAoiParameter*)item;
      CBEType *memberType = getType(param->type);
      CASTExpression *globalMemberPath = new CASTBinaryOp(".", globalPath, new CASTIdentifier(param->name));
      CASTExpression *localMemberPath = new CASTBinaryOp(".", localPath, new CASTIdentifier(param->name));
      CASTIdentifier *loopVar[MAX_DIMENSION];

      prepareIteration(param, &globalMemberPath, &localMemberPath, loopVar, varSource);
      addTo(result, iterate(param,
        memberType->buildTestServerInit(
          globalMemberPath,
          localMemberPath,
          varSource, type, getParameter(param), bufferAvailable),
        loopVar)
      );
    }
  );
  
  return result;
}

CASTStatement *CBEStructType::buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  CASTStatement *result = NULL;

  if ((type!=IN) && (type!=INOUT))
    return NULL;

  forAll(aoi->members, 
    {
      CAoiParameter *param = (CAoiParameter*)item;
      CBEType *memberType = getType(param->type);
      CASTExpression *globalMemberPath = new CASTBinaryOp(".", globalPath, new CASTIdentifier(param->name));
      CASTExpression *localMemberPath = new CASTBinaryOp(".", localPath, new CASTIdentifier(param->name));
      CASTIdentifier *loopVar[MAX_DIMENSION];

      prepareIteration(param, &globalMemberPath, &localMemberPath, loopVar, varSource);
      addTo(result, iterate(param,
        memberType->buildTestServerCheck(
          globalMemberPath,
          localMemberPath,
          varSource, type),
        loopVar)
      );  
    }
  );
  
  return result;
}

CASTStatement *CBEStructType::buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  CASTStatement *result = NULL;

  if (type!=IN)
    return NULL;

  forAll(aoi->members, 
    {
      CAoiParameter *param = (CAoiParameter*)item;
      CBEType *memberType = getType(param->type);
      CASTExpression *globalMemberPath = new CASTBinaryOp(".", globalPath, new CASTIdentifier(param->name));
      CASTExpression *localMemberPath = new CASTBinaryOp(".", localPath, new CASTIdentifier(param->name));
      CASTIdentifier *loopVar[MAX_DIMENSION];

      prepareIteration(param, &globalMemberPath, &localMemberPath, loopVar, varSource);
      addTo(result, iterate(param,
        memberType->buildTestServerRecheck(
          globalMemberPath,
          localMemberPath,
          varSource, type),
        loopVar)
      );  
    }
  );
  
  return result;
}

bool CBEStructType::needsSpecialHandling()

{
  forAll(aoi->members, 
    {
      if (getType(((CAoiParameter*)item)->type)->needsSpecialHandling()) 
        return true;
    }
  );
  
  return false;  
}  
  
bool CBEStructType::needsDirectCopy()

{
  forAll(aoi->members, 
    {
      if (getType(((CAoiParameter*)item)->type)->needsDirectCopy()) 
        return true;
    }
  );
  
  return false;  
}

int CBEStructType::getFlatSize()

{
  const char *currentBitfieldType = NULL;
  int bitfieldBitsRemaining = 0;
  int flatSize = 0;

  forAll(aoi->members, 
    {
      CAoiParameter *thisParam = (CAoiParameter*)item;
      int thisSize = getType(thisParam->type)->getFlatSize();
      int thisAlignment = getType(((CAoiParameter*)item)->type)->getAlignment();

      bool inBitfield = false;
      
      if (thisParam->bits!=0 && currentBitfieldType!=NULL)
        if (!strcmp(thisParam->type->name, currentBitfieldType))
          if (thisParam->bits <= bitfieldBitsRemaining)
            {
              bitfieldBitsRemaining -= thisParam->bits;
              inBitfield = true;
            }

      if (thisParam->refLevel>0)
        warning("Not implemented: Reference types in structs");
        
      for (int i=0;i<thisParam->numBrackets;i++)
        thisSize *= thisParam->dimension[i];

      if (!inBitfield)
        {
          while (flatSize%thisAlignment) 
            flatSize++;
        
          flatSize += thisSize;

          while (flatSize%thisAlignment) 
            flatSize++;
            
          if (thisParam->bits>0)
            {
              currentBitfieldType = thisParam->type->name;
              bitfieldBitsRemaining = getType(thisParam->type)->getFlatSize()*8 - thisParam->bits;
            }
        }    
    }
  );

  int structAlignment = getAlignment();
  while (flatSize%structAlignment)
    flatSize++; 

  return flatSize;
}

int CBEStructType::getAlignment()

{
  /* Structs are aligned according to their strictest member */

  int strictest = 1;

  forAll(aoi->members, 
    {
      int thisAlignment = getType(((CAoiParameter*)item)->type)->getAlignment();
      if (thisAlignment>strictest)
        strictest = thisAlignment;
    }
  );
  
  if (strictest>globals.word_size) 
    strictest = globals.word_size;

  return strictest;
}

CASTExpression *CBEStructType::buildDefaultValue()

{
  CASTExpression *components = NULL;
  forAll(aoi->members,
    {
      CAoiParameter *param = (CAoiParameter*)item;
      CASTExpression *member = getType(param->type)->buildDefaultValue();
      for (int i=0;i<param->numBrackets;i++)
        member = new CASTArrayInitializer(member);
      addTo(components, member);
    }
  );
  return new CASTArrayInitializer(components);
}

CASTExpression *CBEStructType::buildBufferAllocation(CASTExpression *elements)

{
  return buildTypeCast(1, 
    new CASTFunctionOp(
      new CASTIdentifier("CORBA_alloc"), 
      new CASTBinaryOp("*",
        elements,
        new CASTFunctionOp(
          new CASTIdentifier("sizeof"),
          new CASTDeclarationExpression(
            buildDeclaration(NULL)))))
  );
}

CASTStatement *CBEStructType::buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value)

{
  CASTStatement *result = NULL;

  addTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("printf"),
      new CASTStringConstant(false, "{ ")))
  );

  forAll(aoi->members, 
    {
      CAoiParameter *param = (CAoiParameter*)item;
      CBEType *memberType = getType(param->type);
      CASTExpression *globalMemberPath = new CASTBinaryOp(".", globalPath, new CASTIdentifier(param->name));
      CASTExpression *localMemberPath = new CASTBinaryOp(".", localPath, new CASTIdentifier(param->name));
      CASTExpression *memberValue = new CASTBinaryOp(".", value->clone(), new CASTIdentifier(param->name));
      CASTIdentifier *loopVar[MAX_DIMENSION];

      addTo(result, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("printf"),
          new CASTStringConstant(false, mprintf("%s: ", param->name))))
      );
  
      prepareIteration(param, &globalMemberPath, &localMemberPath, loopVar, varSource);
      for (int i=0;i<param->numBrackets;i++)
        memberValue = new CASTIndexOp(memberValue, loopVar[i]->clone());

      addTo(result, iterate(param,
        memberType->buildTestDisplayStmt(
          globalMemberPath,
          localMemberPath,
          varSource, type, memberValue),
        loopVar)
      );
    }
  );

  addTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("printf"),
      new CASTStringConstant(false, "} ")))
  );
  
  return result;
}

CBEParameter *CBEStructType::getFirstMemberWithProperty(const char *property)

{ 
  forAll(aoi->members, 
    if (getParameter(item)->getFirstMemberWithProperty(property))
      return getParameter(item);
  );
  
  return NULL;
}
