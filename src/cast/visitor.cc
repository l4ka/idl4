#include "cast.h"

void CASTBase::acceptAll(CASTVisitor *worker)

{
  CASTBase *iterator = this;
  do {
       iterator->accept(worker);
       iterator = iterator->next;
     } while (iterator!=this);
}  

void CASTBase::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTFile::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTPointer::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTExpression::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTAttributeSpecifier::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTDeclarator::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTIdentifier::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTEllipsis::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTNestedIdentifier::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTUnaryOp::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTCastOp::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTNewOp::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTPostfixOp::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTIndexOp::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTBrackets::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTBinaryOp::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTConditionalExpression::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTArrayInitializer::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTFunctionOp::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTConstant::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTIntegerConstant::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTHexConstant::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTFloatConstant::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTStringConstant::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTCharConstant::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTDefaultConstant::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTStorageClassSpecifier::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTConstOrVolatileSpecifier::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTTypeSpecifier::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTDeclaration::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTEmptyDeclaration::accept(CASTVisitor *worker) 

{
  worker->visit(this); 
}

void CASTStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTPreprocDefine::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTPreprocConditional::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTPreprocInclude::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTPreprocError::accept(CASTVisitor *worker)

{
  worker->visit(this);
}

void CASTComment::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTBlockComment::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTSpacer::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTDeclarationStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTBaseClass::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTAggregateSpecifier::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTEnumSpecifier::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTCompoundStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTExpressionStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTJumpStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTReturnStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTBreakStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTIterationStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTDoWhileStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTWhileStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTForStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTSelectionStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTIfStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTAsmInstruction::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTAsmConstraint::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTAsmStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}
 
void CASTDeclarationExpression::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTExternStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTEmptyStatement::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTBooleanConstant::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTBuiltinConstant::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTConstructorDeclaration::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTEmptyConstant::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTDeclarationSpecifier::accept(CASTVisitor *worker) 

{ 
  worker->visit(this); 
}

void CASTLabeledExpression::accept(CASTVisitor *worker)

{
  worker->visit(this);
}

void CASTGotoStatement::accept(CASTVisitor *worker)

{
  worker->visit(this);
}

void CASTCaseStatement::accept(CASTVisitor *worker)

{
  worker->visit(this);
}

void CASTSwitchStatement::accept(CASTVisitor *worker)

{
  worker->visit(this);
}

void CASTLabelStatement::accept(CASTVisitor *worker)

{
  worker->visit(this);
}

void CASTTemplateStatement::accept(CASTVisitor *worker)

{
  worker->visit(this);
}

void CASTTemplateTypeDeclaration::accept(CASTVisitor *worker)

{
  worker->visit(this);
}

void CASTTemplateTypeIdentifier::accept(CASTVisitor *worker)

{
  worker->visit(this);
}

void CASTOperatorIdentifier::accept(CASTVisitor *worker)

{
  worker->visit(this);
}

void CASTRef::accept(CASTVisitor *worker)

{
  worker->visit(this);
}

void CASTIndirectTypeSpecifier::accept(CASTVisitor *worker)

{
  worker->visit(this);
}

void CASTStatementExpression::accept(CASTVisitor *worker)

{
  worker->visit(this);
}

void CASTContinueStatement::accept(CASTVisitor *worker)

{
  worker->visit(this);
}

void CASTPreprocUndef::accept(CASTVisitor *worker)

{
  worker->visit(this);
}
