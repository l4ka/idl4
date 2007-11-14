#include "check.h"

void CAoiCheckVisitor::visit(CAoiDeclarator *peer)

{
  basicCheck(CAoiBase, "CAoiDeclarator");
  
  assert((peer->refLevel>=0) && (peer->refLevel<3));
  assert((peer->numBrackets>=0) && (peer->numBrackets<=MAX_DIMENSION));
  for (int i=0;i<peer->numBrackets;i++)
    assert((peer->dimension[i]>=0) || 
           ((peer->dimension[i]==-1) && (i==(peer->numBrackets-1))));
    
  assert(peer->context);
    
  finishCheck();  
}

void CAoiCheckVisitor::visit(CAoiList *peer)

{
  int steps;
  CAoiBase *tmp;

  basicCheck(CAoiBase, "CAoiList");
    
  assert(!peer->name);
  
  tmp = peer->next;
  steps = 0;
  while (tmp != peer)
    {
      assert(tmp);
      assert((steps++)<10000);
      assert(!tmp->isEndOfList());
      tmp->accept(this);
      tmp = tmp->next;
    }  

  tmp = peer->prev;
  while (tmp != peer)
    {
      assert(tmp);
      assert((steps--)>0);
      assert(!tmp->isEndOfList());
      tmp = tmp->prev;
    }  
    
  assert(!steps);  
    
  finishCheck();  
}

void CAoiCheckVisitor::visit(CAoiScope *peer)

{
  basicCheck(CAoiBase, "CAoiScope");
  
  assert(peer->submodules);
  peer->submodules->accept(this);
  assertAllItems(peer->submodules, item->isScope());

  assert(peer->interfaces);
  peer->interfaces->accept(this);
  assertAllItems(peer->interfaces, item->isInterface());

  assert(peer->types);
  peer->types->accept(this);
  assertAllItems(peer->types, item->isType());

  assert(peer->attributes);
  peer->attributes->accept(this);
  assertAllItems(peer->attributes, item->isAttribute());

  assert(peer->constants);
  peer->constants->accept(this);
  assertAllItems(peer->constants, item->isConstant());
  
  assert(peer->operations);
  peer->operations->accept(this);
  assertAllItems(peer->operations, item->isOperation());
  
  assert(peer->exceptions);
  peer->exceptions->accept(this);
  assertAllItems(peer->exceptions, item->isException());

  assert(peer->includes);
  peer->includes->accept(this);
  
  assert(peer->context);
  
  assert(peer->scopedName);
  
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiInterface *peer)

{
  basicCheck(CAoiScope, "CAoiInterface");

  assert(peer->superclasses);  
  peer->superclasses->accept(this);
  assertAllItems(peer->superclasses, item->isReference());
  assertAllItems(peer->superclasses, (((CAoiRef*)item)->ref)->isInterface());
  
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiStructType *peer)

{
  basicCheck(CAoiType, "CAoiStructType");

  assert(peer->members);  
  peer->members->accept(this);
  assertAllItems(peer->members, item->isParameter());
  
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiUnionType *peer)

{
  basicCheck(CAoiType, "CAoiUnionType");

  assert(peer->members);  
  peer->members->accept(this);
  assertAllItems(peer->members, item->isParameter());
  
  /* For non-CORBA unions, it is legal to have no switch element */
  if (peer->switchType)
    {
      assert(peer->switchType->isType());
      peer->switchType->accept(this);
    }  
  
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiEnumType *peer)

{
  basicCheck(CAoiType, "CAoiEnumType");

  assert(peer->members);  
  peer->members->accept(this);
  assertAllItems(peer->members, item->isReference());
  assertAllItems(peer->members, (((CAoiRef*)item)->ref)->isConstant());
  
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiAliasType *peer)

{
  basicCheck(CAoiType, "CAoiAliasType");
  
  assert((peer->numBrackets>=0) && (peer->numBrackets<=MAX_DIMENSION));
  for (int i=0;i<peer->numBrackets;i++)
    assert((peer->dimension[i]>0) || 
           ((peer->dimension[i]==-1) && (i==(peer->numBrackets-1))));
    
  assert(peer->ref);
  assert(peer->ref->isType());
  peer->ref->accept(this);
    
  finishCheck();  
}

void CAoiCheckVisitor::visit(CAoiPointerType *peer)

{
  basicCheck(CAoiType, "CAoiPointerType");
  
  assert(peer->ref);
  assert(peer->ref->isType());
  peer->ref->accept(this);
    
  finishCheck();  
}

void CAoiCheckVisitor::visit(CAoiObjectType *peer)

{
  basicCheck(CAoiType, "CAoiObjectType");
  
  if (peer->ref)
    assert(peer->ref->isInterface());
  /* do not visit peer (endless recursion!) */
    
  finishCheck();  
}

void CAoiCheckVisitor::visit(CAoiConstant *peer)

{
  basicCheck(CAoiBase, "CAoiConstant");

  assert(peer->parentScope && peer->parentScope->isScope());

  assert(peer->type && peer->type->isType());  
  peer->type->accept(this);
  
  assert(peer->value);
  assert(peer->value && peer->value->isConstBase());
  peer->value->accept(this);
  
  assert(peer->context);
  
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiParameter *peer)

{
  basicCheck(CAoiDeclarator, "CAoiParameter");

  assert(!(peer->flags&(~(FLAG_IN+FLAG_OUT+FLAG_REF+FLAG_MAP+FLAG_GRANT+
                          FLAG_WRITABLE+FLAG_NOCACHE+FLAG_L1_ONLY+
                          FLAG_ALL_CACHES+FLAG_FOLLOW+FLAG_NOXFER+
                          FLAG_READONLY+FLAG_PREALLOC))));  
  assert(peer->type && peer->type->isType());
  peer->type->accept(this);
  
  assert(peer->parent && ((peer->parent->isOperation()) || 
                          (peer->parent->isStruct()) ||
                          (peer->parent->isException()) || 
                          (peer->parent->isUnion()) ||
                          (peer->parent->isScope())));
  
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiUnionElement *peer)

{
  basicCheck(CAoiParameter, "CAoiUnionElement");

  /* For non-CORBA unions, it is legal to have no discriminator */
  if (peer->discrim)
    peer->discrim->accept(this);
  
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiOperation *peer)

{
  basicCheck(CAoiBase, "CAoiOperation");

  assert(peer->parameters);  
  peer->parameters->accept(this);
  assertAllItems(peer->parameters, item->isParameter());

  assert(!(peer->flags&(~(FLAG_ONEWAY+FLAG_LOCAL+FLAG_KERNELMSG))));

  assert(peer->exceptions);  
  peer->exceptions->accept(this);
  assertAllItems(peer->exceptions, item->isReference());
  assertAllItems(peer->exceptions, (((CAoiRef*)item)->ref)->isException());

  assert(peer->returnType && peer->returnType->isType());
  peer->returnType->accept(this);
  
  assert(peer->parent && (peer->parent->isScope()));
  
  assert(peer->context);

  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiProperty *peer)

{
  basicCheck(CAoiBase, "CAoiProperty");
  
  if (peer->constValue)
    assert(peer->constValue->isConstBase());
    
  assert(peer->context);

  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiConstString *peer)

{
  basicCheck(CAoiConstBase, "CAoiConstString"); 
  
  assert(peer->value); 
  
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiModule *peer)

{
  basicCheck(CAoiScope, "CAoiModule"); 
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiRootScope *peer)

{
  basicCheck(CAoiScope, "CAoiRootScope"); 
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiType *peer)

{
  basicCheck(CAoiBase, "CAoiType"); 
  
  assert(peer->parentScope);
  assert(peer->parentScope->isScope());
  
  assert(peer->context);

  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiExceptionType *peer)

{
  basicCheck(CAoiStructType, "CAoiExceptionType");
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiIntegerType *peer)

{
  basicCheck(CAoiType, "CAoiIntegerType"); 
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiWordType *peer)

{
  basicCheck(CAoiType, "CAoiWordType"); 
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiFloatType *peer)

{
  basicCheck(CAoiType, "CAoiFloatType"); 
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiStringType *peer)

{
  basicCheck(CAoiType, "CAoiStringType"); 
  
  assert(peer->elementType);
  peer->elementType->accept(this);
  
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiFixedType *peer)

{
  basicCheck(CAoiType, "CAoiFixedType"); 
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiSequenceType *peer)

{
  basicCheck(CAoiType, "CAoiSequenceType"); 
  
  assert(peer->elementType);
  assert(peer->elementType->isType());
  
  peer->elementType->accept(this);
  
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiCustomType *peer)

{
  basicCheck(CAoiType, "CAoiCustomType"); 
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiRef *peer)

{
  basicCheck(CAoiBase, "CAoiRef"); 
  
  assert(peer->ref); 
  peer->ref->accept(this);
  
  assert(peer->context);

  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiAttribute *peer)

{
  basicCheck(CAoiParameter, "CAoiAttribute"); 
  finishCheck();
}

void CAoiCheckVisitor::visit(CAoiContext *peer)

{
  assert(peer->file);
  assert(peer->line);
  assert(peer->lineNo>=0);
  assert(peer->pos>=0);
}
