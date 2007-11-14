#include "ops.h"

CASTStatement *CBEUnionType::buildDefinition()

{
  CASTStatement *result = NULL;

  if (aoi->flags & FLAG_DONTDEFINE)
    return NULL;

  addTo(result, buildConditionalDefinition(
    new CASTDeclarationStatement(
      new CASTDeclaration(buildSpecifier(), ANONYMOUS)))
  );    
  
  addTo(result, new CASTSpacer());
  
  return result;
}

CASTDeclarationSpecifier *CBEUnionType::buildSpecifier()

{
  if (aoi->switchType)
    {
      /* Build the actual union (this is going to be an element of the
         outer struct */

      CASTDeclarationStatement *unionMembers = NULL;
      forAll(aoi->members, addTo(unionMembers, new CASTDeclarationStatement(getParameter(item)->buildDeclaration())));
  
      CASTDeclaration *unionDeclaration = new CASTDeclaration(
                       new CASTAggregateSpecifier("union", ANONYMOUS, unionMembers),
                       knitDeclarator("_u"));

      /* Build the outer struct, which contains the discriminator and the
         actual union */
  
      CASTDeclarationStatement *structMembers = NULL;

      addTo(structMembers, new CASTDeclarationStatement(getType(aoi->switchType)->buildDeclaration(knitDeclarator("_d"))));
      addTo(structMembers, new CASTDeclarationStatement(unionDeclaration));
                 
      return new CASTAggregateSpecifier("struct", buildIdentifier(), structMembers);
    }
    
  /* Non-CORBA union */  

  CASTDeclarationStatement *unionMembers = NULL;
  forAll(aoi->members, addTo(unionMembers, new CASTDeclarationStatement(getParameter(item)->buildDeclaration())));
  
  return new CASTAggregateSpecifier("union", buildIdentifier(), unionMembers);
}

CASTDeclaration *CBEUnionType::buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound) 

{ 
  if (aoi->name) 
    {
      CASTDeclarationSpecifier *spec = NULL;
      addTo(spec, new CASTStorageClassSpecifier((aoi->switchType) ? "struct" : "union"));
      addTo(spec, new CASTTypeSpecifier(buildIdentifier()));
      return new CASTDeclaration(spec, decl, compound);
    } else return new CASTDeclaration(buildSpecifier(), decl, compound); 
}

int CBEUnionType::getArgIndirLevel(CBEDeclType declType)

{ 
  if ((declType==IN) || (declType==INOUT) || (declType==OUT))
    return 1;
    
  return 0;
}

CBEMarshalOp *CBEUnionType::buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags)

{
  forAll(aoi->members, 
    {
      CBEType *memberType = getType(((CAoiParameter*)item)->type);

      if (memberType->needsSpecialHandling())
        panic("Cannot marshal union %s: Member %s needs special handling", aoi->name, item->name);

      if (!memberType->needsDirectCopy())
        panic("Cannot marshal union %s: Member %s does not need to be copied", aoi->name, item->name);
    }
  );

  return new CBEOpSimpleCopy(connection, channels, rvalue, originalType, getFlatSize(), name, param, flags);
}

CBEUnionElement *CBEUnionType::getLargestMember()

{
  CAoiUnionElement *largestMember = NULL;
  int largestSize = 0;
  
  forAll(aoi->members,
    {
      CAoiUnionElement *thisMember = (CAoiUnionElement*)item;
      int thisSize = getType(thisMember->type)->getFlatSize();
      if (thisSize>largestSize)
        {
          largestSize = thisSize;
          largestMember = thisMember;
        }
    }
  );
  
  assert(largestMember);
  return getUnionElement(largestMember);
}

CASTStatement *CBEUnionType::buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param)

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

  CAoiUnionElement *elem = getLargestMember()->aoi;
  CBEType *memberType = getType(elem->type);
  CASTExpression *globalMemberPath = new CASTBinaryOp(".", globalPath, new CASTIdentifier(elem->name));
  CASTExpression *localMemberPath = new CASTBinaryOp(".", localPath, new CASTIdentifier(elem->name));
      
  addTo(result, memberType->buildTestClientInit(
    globalMemberPath,
    localMemberPath,
    varSource, type, getLargestMember())
  );
  
  return result;
}

CASTStatement *CBEUnionType::buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{ 
  return NULL;
}

CASTStatement *CBEUnionType::buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{ 
  return NULL;
}

CASTStatement *CBEUnionType::buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  CASTStatement *result = NULL;

  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  CAoiUnionElement *elem = getLargestMember()->aoi;
  CBEType *memberType = getType(elem->type);
  CASTExpression *globalMemberPath = new CASTBinaryOp(".", globalPath, new CASTIdentifier(elem->name));
  CASTExpression *localMemberPath = new CASTBinaryOp(".", localPath, new CASTIdentifier(elem->name));

  addTo(result, 
    memberType->buildTestClientCheck(
      globalMemberPath,
      localMemberPath,
      varSource, type)
  );
  
  return result;
}

CASTStatement *CBEUnionType::buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable)

{
  CASTStatement *result = NULL;

  if ((type!=INOUT) && (type!=OUT))
    return NULL;

  CAoiUnionElement *elem = getLargestMember()->aoi;
  CBEType *memberType = getType(elem->type);
  CASTExpression *globalMemberPath = new CASTBinaryOp(".", globalPath, new CASTIdentifier(elem->name));
  CASTExpression *localMemberPath = new CASTBinaryOp(".", localPath, new CASTIdentifier(elem->name));

  addTo(result, 
    memberType->buildTestServerInit(
      globalMemberPath,
      localMemberPath,
      varSource, type, getLargestMember(), bufferAvailable)
  );
  
  return result;
}

CASTStatement *CBEUnionType::buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  CASTStatement *result = NULL;

  if ((type!=IN) && (type!=INOUT))
    return NULL;

  CAoiUnionElement *elem = getLargestMember()->aoi;
  CBEType *memberType = getType(elem->type);
  CASTExpression *globalMemberPath = new CASTBinaryOp(".", globalPath, new CASTIdentifier(elem->name));
  CASTExpression *localMemberPath = new CASTBinaryOp(".", localPath, new CASTIdentifier(elem->name));

  addTo(result, 
    memberType->buildTestServerCheck(
      globalMemberPath,
      localMemberPath,
      varSource, type)
  );

  return result;
}

CASTStatement *CBEUnionType::buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type)

{
  CASTStatement *result = NULL;

  if (type!=IN)
    return NULL;

  CAoiUnionElement *elem = getLargestMember()->aoi;
  CBEType *memberType = getType(elem->type);
  CASTExpression *globalMemberPath = new CASTBinaryOp(".", globalPath, new CASTIdentifier(elem->name));
  CASTExpression *localMemberPath = new CASTBinaryOp(".", localPath, new CASTIdentifier(elem->name));

  addTo(result, 
    memberType->buildTestServerRecheck(
      globalMemberPath,
      localMemberPath,
      varSource, type)
  );

  return result;
}

bool CBEUnionType::needsSpecialHandling()

{
  forAll(aoi->members, 
    {
      if (getType(((CAoiParameter*)item)->type)->needsSpecialHandling()) 
        return true;
    }
  );
  
  return false;  
}  
  
bool CBEUnionType::needsDirectCopy()

{
  forAll(aoi->members, 
    {
      if (getType(((CAoiParameter*)item)->type)->needsDirectCopy()) 
        return true;
    }
  );
  
  return false;  
}

int CBEUnionType::getFlatSize()

{
  int flatSize = 0;

  forAll(aoi->members, 
    {
      CAoiUnionElement *thisElement = (CAoiUnionElement*)item;
      int thisSize = getType(thisElement->type)->getFlatSize();
      int thisAlignment = getType(thisElement->type)->getAlignment();

      if (thisElement->refLevel>0)
        warning("Not implemented: Reference types in unions");
        
      if (thisElement->numBrackets>0)
        warning("Not implemented: Arrays in unions");
        
      while (thisSize%thisAlignment) 
        thisSize++;
        
      if (flatSize<thisSize)
        flatSize = thisSize;
    }
  );

  return flatSize;
}

int CBEUnionType::getAlignment()

{
  /* Unions are aligned according to their strictest member */

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

CASTExpression *CBEUnionType::buildDefaultValue()

{
  CBEUnionElement *largestMember = getLargestMember();
  
  return new CASTArrayInitializer(
    new CASTLabeledExpression(
      new CASTIdentifier(largestMember->aoi->name), 
      getType(largestMember->aoi->type)->buildDefaultValue())
  );
}

CASTStatement *CBEUnionType::buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value)

{
  CAoiUnionElement *largestMember = getLargestMember()->aoi;
  CASTExpression *globalMemberPath = new CASTBinaryOp(".", globalPath, new CASTIdentifier(largestMember->name));
  CASTExpression *localMemberPath = new CASTBinaryOp(".", localPath, new CASTIdentifier(largestMember->name));
  CASTExpression *memberValue = new CASTBinaryOp(".", value, new CASTIdentifier(largestMember->name));
      
  return getType(largestMember->type)->buildTestDisplayStmt(
    globalMemberPath,
    localMemberPath,
    varSource, type, memberValue
  );
}
