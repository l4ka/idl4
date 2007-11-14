#include "aoi.h"

void CAoiBase::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiList::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiConstInt::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiConstString::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiConstChar::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiConstFloat::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiConstDefault::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiDeclarator::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiScope::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiModule::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiInterface::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiRootScope::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiStructType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiUnionType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiEnumType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiExceptionType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiAliasType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}

void CAoiPointerType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}

void CAoiObjectType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiIntegerType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiWordType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiStringType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiFloatType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}

void CAoiFixedType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiSequenceType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiCustomType::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiRef::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiConstant::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiParameter::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiUnionElement::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiProperty::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiOperation::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
  
void CAoiAttribute::accept(CAoiVisitor *worker)

{
  worker->visit(this);
}
