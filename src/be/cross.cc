#include "cross.h"

void CAoiCrossVisitor::visit(CAoiList *aoi)

{
  if (!aoi->isEmpty())
    {
      CAoiBase *iterator = aoi->getFirstElement();
      while (!iterator->isEndOfList())
        {
          iterator->accept(this);
          iterator = iterator->next;
        }
    } 
} 

void CAoiCrossVisitor::visit(CAoiConstant *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEConstant(aoi);

  aoi->value->accept(this);
}

void CAoiCrossVisitor::visit(CAoiAliasType *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEAliasType(aoi);

  aoi->ref->accept(this);
}

void CAoiCrossVisitor::visit(CAoiPointerType *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEPointerType(aoi);

  aoi->ref->accept(this);
}

void CAoiCrossVisitor::visit(CAoiObjectType *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEObjectType(aoi);

  if (aoi->ref)
    aoi->ref->accept(this);
}

void CAoiCrossVisitor::visit(CAoiParameter *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEParameter(aoi);
    
  aoi->type->accept(this);
}

void CAoiCrossVisitor::visit(CAoiAttribute *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEAttribute(aoi);
}

void CAoiCrossVisitor::visit(CAoiOperation *aoi)

{
  if (!aoi->peer)
    {
      aoi->peer = new CBEOperation(aoi);
      aoi->returnType->accept(this);
      aoi->parameters->accept(this);
      aoi->exceptions->accept(this);
      
      CBEDependencyList *depList = new CBEDependencyList();
      forAll(aoi->parameters, getParameter(item)->registerDependencies(depList));
      if (!depList->commit())
        semanticError(aoi->context, "circular dependency between parameters");
        
      int maxPrio = 0;
      forAll(aoi->parameters, 
        if (getParameter(item)->priority>maxPrio)
          maxPrio = getParameter(item)->priority;
      );
      
      CBEList *sortedParams = getOperation(aoi)->sortedParams;
      assert(sortedParams->isEmpty());
      for (int i=0;i<=maxPrio;i++)
        forAll(aoi->parameters,
          if (getParameter(item)->priority == i)
            sortedParams->add(getParameter(item));
        );  
    }  
}

void CAoiCrossVisitor::visit(CAoiUnionType *aoi)

{ 
  if (!aoi->peer)
    aoi->peer = new CBEUnionType(aoi);

  if (aoi->switchType)
    aoi->switchType->accept(this);
    
  aoi->members->accept(this);
}

void CAoiCrossVisitor::visit(CAoiUnionElement *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEUnionElement(aoi);

  if (aoi->discrim)
    aoi->discrim->accept(this);
    
  aoi->type->accept(this);
}

void CAoiCrossVisitor::visit(CAoiConstInt *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEConstInt(aoi);
}

void CAoiCrossVisitor::visit(CAoiConstString *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEConstString(aoi);
}

void CAoiCrossVisitor::visit(CAoiConstChar *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEConstChar(aoi);
}

void CAoiCrossVisitor::visit(CAoiConstFloat *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEConstFloat(aoi);
}

void CAoiCrossVisitor::visit(CAoiConstDefault *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEConstDefault(aoi);
}

void CAoiCrossVisitor::visit(CAoiModule *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEModule(aoi);

  aoi->constants->accept(this);
  aoi->types->accept(this);
  aoi->attributes->accept(this);
  aoi->exceptions->accept(this);
  aoi->operations->accept(this);
  aoi->submodules->accept(this);
  aoi->interfaces->accept(this);
}

void CAoiCrossVisitor::visit(CAoiInterface *aoi)

{
  if (!aoi->peer)
    {
      CAoiList *singleSuperclasses = aoi->getAllSuperclasses();
      CBEList *inheritedOps = new CBEList();
      
      forAll(singleSuperclasses, 
        {
          CAoiInterface *superclass = (CAoiInterface*)((CAoiRef*)item)->ref;
          CAoiOperation *op = (CAoiOperation*)superclass->operations->getFirstElement();
          
          if (superclass != aoi)
            while (!op->isEndOfList())
              {
                inheritedOps->add(new CBEInheritedOperation(op, aoi));
                op = (CAoiOperation*)op->next;
              }
        }
      );

      aoi->peer = new CBEInterface(aoi, inheritedOps);
    }  

  aoi->constants->accept(this);
  aoi->types->accept(this);
  aoi->attributes->accept(this);
  aoi->exceptions->accept(this);
  aoi->operations->accept(this);
  aoi->submodules->accept(this);
  aoi->interfaces->accept(this);
  aoi->superclasses->accept(this);
}

void CAoiCrossVisitor::visit(CAoiRootScope *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBERootScope(aoi);

  aoi->constants->accept(this);
  aoi->types->accept(this);
  aoi->attributes->accept(this);
  aoi->exceptions->accept(this);
  aoi->operations->accept(this);
  aoi->submodules->accept(this);
  aoi->interfaces->accept(this);
}

void CAoiCrossVisitor::visit(CAoiStructType *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEStructType(aoi);
    
  aoi->members->accept(this);
}

void CAoiCrossVisitor::visit(CAoiEnumType *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEEnumType(aoi);

  aoi->members->accept(this);
}

void CAoiCrossVisitor::visit(CAoiExceptionType *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEExceptionType(aoi);

  aoi->members->accept(this);
}

void CAoiCrossVisitor::visit(CAoiRef *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBERef(aoi);
  
  aoi->ref->accept(this);
}

void CAoiCrossVisitor::visit(CAoiIntegerType *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEIntegerType(aoi);
}

void CAoiCrossVisitor::visit(CAoiWordType *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEWordType(aoi);
}

void CAoiCrossVisitor::visit(CAoiFloatType *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEFloatType(aoi);
}

void CAoiCrossVisitor::visit(CAoiStringType *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEStringType(aoi);
    
  aoi->elementType->accept(this);
}

void CAoiCrossVisitor::visit(CAoiFixedType *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBEFixedType(aoi);
}

void CAoiCrossVisitor::visit(CAoiSequenceType *aoi)

{
  if (!aoi->peer)
    aoi->peer = new CBESequenceType(aoi);
    
  aoi->elementType->accept(this);
}

void CAoiCrossVisitor::visit(CAoiCustomType *aoi)

{
  if (!aoi->peer)
    {
      if (!strcmp(aoi->identifier, "threadid"))
        {
          aoi->peer = new CBEThreadIDType(aoi);
        }
      else if (!strcmp(aoi->identifier, "fpage"))
        {
          aoi->peer = new CBEFpageType(aoi);
        }
      else panic("Unknown custom type: %s", aoi->identifier);
    }  
}
