#include "cast.h"

#define buildClone(a) (((a)!=NULL) ? ((a)->clone()) : NULL)
#define buildAllClones(a) (((a)!=NULL) ? ((a)->cloneAll()) : NULL)

CASTBase *CASTBase::cloneAll()

{
  CASTBase *iterator = this, *result = NULL;

  do {
       if (result)
         result->add(iterator->clone());
         else result = iterator->clone();
         
       iterator = iterator->next;
     } while (iterator!=this);  
     
  return result;
}    

CASTBase *CASTBase::clone()

{
  panic("Cannot clone: CASTBase");
  return NULL;
}

CASTExpression *CASTExpression::clone()

{
  panic("Cannot clone: CASTExpression");
  return NULL;
}

CASTDeclarator *CASTDeclarator::clone()

{
  CASTDeclarator *decl;
  decl = new CASTDeclarator((CASTIdentifier *) buildAllClones(identifier),
			    (CASTDeclaration *) buildAllClones(parameters),
			    (CASTPointer *) buildAllClones(pointer),
			    (CASTDeclarator *) buildAllClones(subdecl),
			    (CASTExpression *) buildAllClones(arrayDims), 
			    (CASTAttributeSpecifier *) buildAllClones(attributes),
			    (CASTDeclarationSpecifier *) buildAllClones(qualifiers),
			    (CASTStringConstant *) buildAllClones(asmName));
  decl->initializer = (CASTExpression *) buildAllClones(initializer);
  return decl;
}

CASTIdentifier *CASTIdentifier::clone()

{
  return new CASTIdentifier(mprintf("%s", identifier));
}

CASTDeclarationSpecifier *CASTDeclarationSpecifier::clone()

{
  panic("Cannot clone: CASTDeclarationSpecifer");
  return NULL;
}

CASTStorageClassSpecifier *CASTStorageClassSpecifier::clone()

{
  return new CASTStorageClassSpecifier(mprintf("%s", keyword));
}

CASTConstOrVolatileSpecifier *CASTConstOrVolatileSpecifier::clone()

{
  return new CASTConstOrVolatileSpecifier(mprintf("%s", keyword));
}

CASTDeclaration *CASTDeclaration::clone()

{
  return new CASTDeclaration((CASTDeclarationSpecifier *) buildAllClones(specifiers),
			     (CASTDeclarator *) buildAllClones(declarators),
			     (CASTCompoundStatement *) buildAllClones(compound),
			     (CASTExpression *) buildAllClones(baseInit));
} 

CASTExpressionStatement *CASTExpressionStatement::clone()

{
  return new CASTExpressionStatement((CASTExpression *) buildAllClones(expression));
}

CASTCompoundStatement *CASTCompoundStatement::clone()

{
  return new CASTCompoundStatement((CASTStatement *) buildAllClones(statements));
}

CASTPointer *CASTPointer::clone()

{
  return new CASTPointer();
}

CASTStatement *CASTStatement::clone()

{
  panic("Cannot clone: CASTStatement");
  return NULL;
}

CASTTypeSpecifier *CASTTypeSpecifier::clone()

{
  return new CASTTypeSpecifier((CASTIdentifier *) buildAllClones(identifier));
}

CASTUnaryOp *CASTUnaryOp::clone()

{
  return new CASTUnaryOp(mprintf("%s", op),
			 (CASTExpression *) buildAllClones(leftExpr));
}

CASTBinaryOp *CASTBinaryOp::clone()

{
  return new CASTBinaryOp(mprintf("%s", op),
			  (CASTExpression *) buildAllClones(leftExpr),
			  (CASTExpression *) buildAllClones(rightExpr));
}  

CASTIndexOp *CASTIndexOp::clone()

{
  return new CASTIndexOp((CASTExpression *) buildAllClones(baseExpr),
			 (CASTExpression *) buildAllClones(indexExpr));
}

CASTBrackets *CASTBrackets::clone()

{
  return new CASTBrackets((CASTExpression *) buildAllClones(expression));
}

CASTFunctionOp *CASTFunctionOp::clone()

{
  return new CASTFunctionOp((CASTExpression *) buildAllClones(function),
			    (CASTExpression *) buildAllClones(arguments));
}

CASTIntegerConstant *CASTIntegerConstant::clone()

{
  return new CASTIntegerConstant(value, isUnsigned, isLong, isLongLong);
}

CASTHexConstant *CASTHexConstant::clone()

{
  return new CASTHexConstant(value, isUnsigned, isLong, isLongLong);
}

CASTAttributeSpecifier *CASTAttributeSpecifier::clone()

{
  return new CASTAttributeSpecifier((CASTIdentifier *) buildAllClones(name),
				    (CASTExpression *) buildAllClones(value));
}

CASTNestedIdentifier *CASTNestedIdentifier::clone()

{
  return new CASTNestedIdentifier((CASTIdentifier *) buildAllClones(scopeName),
				  (CASTIdentifier *) buildAllClones(ident));
}

CASTDeclarationExpression *CASTDeclarationExpression::clone()

{
  return new CASTDeclarationExpression((CASTDeclaration *) buildAllClones(contents));
}

CASTBooleanConstant *CASTBooleanConstant::clone()

{
  return new CASTBooleanConstant(value);
}

CASTBuiltinConstant *CASTBuiltinConstant::clone()

{
  return new CASTBuiltinConstant(mprintf("%s", name));
}

CASTEmptyStatement *CASTEmptyStatement::clone()

{
  return new CASTEmptyStatement();
}

CASTCastOp *CASTCastOp::clone()

{
  return new CASTCastOp((CASTDeclaration *) buildAllClones(target),
			(CASTExpression *) buildAllClones(leftExpr),
			type);
}

CASTNewOp *CASTNewOp::clone()

{
  return new CASTNewOp((CASTDeclarationSpecifier *) buildAllClones(type),
		       (CASTExpression *) buildAllClones(leftExpr));
}

CASTPostfixOp *CASTPostfixOp::clone()

{
  return new CASTPostfixOp(mprintf("%s", op),
                           (CASTExpression *) buildAllClones(leftExpr));
}

CASTConstructorDeclaration *CASTConstructorDeclaration::clone()

{
  CASTConstructorDeclaration *c =
    new CASTConstructorDeclaration((CASTDeclarationSpecifier *) buildAllClones(specifiers),
				   (CASTDeclarator *) buildAllClones(declarators),
				   (CASTCompoundStatement *) buildAllClones(compound));
  c->setBaseInit(baseInit);
  return c;
} 

CASTTemplateTypeDeclaration *CASTTemplateTypeDeclaration::clone()

{
  return new CASTTemplateTypeDeclaration((CASTDeclarator *) buildAllClones(declarators));
}

CASTTemplateTypeIdentifier *CASTTemplateTypeIdentifier::clone()

{
  return new CASTTemplateTypeIdentifier(mprintf("%s", identifier),
					(CASTExpression *) buildAllClones(arguments));
}

CASTOperatorIdentifier *CASTOperatorIdentifier::clone()

{
  return new CASTOperatorIdentifier(mprintf("%s", identifier));
}

CASTRef *CASTRef::clone()

{
  return new CASTRef();
}

CASTFloatConstant *CASTFloatConstant::clone()

{
  return new CASTFloatConstant(value);
}

CASTStringConstant *CASTStringConstant::clone()

{
  return new CASTStringConstant(isWide, mprintf("%s", value));
}

CASTCharConstant *CASTCharConstant::clone()

{
  return new CASTCharConstant(isWide, value);
}

CASTLabeledExpression *CASTLabeledExpression::clone()

{
  return new CASTLabeledExpression((CASTIdentifier *) buildAllClones(name),
				   (CASTExpression *) buildAllClones(value));
}

CASTArrayInitializer *CASTArrayInitializer::clone()

{
  return new CASTArrayInitializer((CASTExpression *) buildAllClones(elements));
}

CASTComment *CASTComment::clone()

{
  return new CASTComment(mprintf("%s", text));
}

CASTBlockComment *CASTBlockComment::clone()

{
  return new CASTBlockComment(mprintf("%s", text));
}

CASTDeclarationStatement *CASTDeclarationStatement::clone()

{
  return new CASTDeclarationStatement((CASTDeclaration *) buildAllClones(declaration),
				      visibility);
}

CASTJumpStatement *CASTJumpStatement::clone()

{
  return new CASTJumpStatement(NULL);
}

CASTBreakStatement *CASTBreakStatement::clone()

{
  return new CASTBreakStatement(NULL);
}

CASTContinueStatement *CASTContinueStatement::clone()

{
  return new CASTContinueStatement(NULL);
}

CASTGotoStatement *CASTGotoStatement::clone()

{
  return new CASTGotoStatement((CASTIdentifier *) buildAllClones(label));
}

CASTPreprocError *CASTPreprocError::clone()

{
  return new CASTPreprocError((CASTIdentifier *) buildAllClones(message));
}

CASTPreprocInclude *CASTPreprocInclude::clone()

{
  return new CASTPreprocInclude((CASTIdentifier *) buildAllClones(name));
}

CASTConditionalExpression *CASTConditionalExpression::clone()

{
  return new CASTConditionalExpression((CASTExpression *) buildAllClones(expression),
				       (CASTExpression *) buildAllClones(resultIfTrue),
				       (CASTExpression *) buildAllClones(resultIfFalse));
}

CASTStatementExpression *CASTStatementExpression::clone()

{
  return new CASTStatementExpression((CASTStatement *) buildAllClones(contents));
}

CASTIndirectTypeSpecifier *CASTIndirectTypeSpecifier::clone()

{
  return new CASTIndirectTypeSpecifier(mprintf("%s", operatorName),
				       (CASTExpression *) buildAllClones(typeExpr));
}

CASTEnumSpecifier *CASTEnumSpecifier::clone()

{
  return new CASTEnumSpecifier((CASTIdentifier *) buildAllClones(identifier),
			       (CASTDeclarator *) buildAllClones(elements));
}

CASTAggregateSpecifier *CASTAggregateSpecifier::clone()

{
  CASTAggregateSpecifier *n =
    new CASTAggregateSpecifier(mprintf("%s", type),
			       (CASTIdentifier *) buildAllClones(identifier),
			       (CASTStatement *) buildAllClones(declarations));
  n->setAttributes((CASTAttributeSpecifier *) buildAllClones(attributes));
  n->setParents((CASTBaseClass *) buildAllClones(parents));
  return n;
}

CASTAsmConstraint *CASTAsmConstraint::clone()

{
  return new CASTAsmConstraint(mprintf("%s", name),
			       (CASTExpression *) buildAllClones(value));
}

CASTBaseClass *CASTBaseClass::clone()

{
  return new CASTBaseClass((CASTIdentifier *) buildAllClones(identifier),
			   visibility,
			   (CASTStorageClassSpecifier *) buildAllClones(specifiers));
}

CASTAsmStatement *CASTAsmStatement::clone()

{
  return new CASTAsmStatement((CASTAsmInstruction *) buildAllClones(instructions),
			      (CASTAsmConstraint *) buildAllClones(inConstraints),
			      (CASTAsmConstraint *) buildAllClones(outConstraints),
			      buildAllClones(clobberConstraints),
			      (CASTConstOrVolatileSpecifier *) buildAllClones(specifier));
}

CASTCaseStatement *CASTCaseStatement::clone()

{
  return new CASTCaseStatement((CASTStatement *) buildAllClones(statements),
			       (CASTExpression *) buildAllClones(discrimValue),
			       (CASTExpression *) buildAllClones(discrimEnd));
}

CASTExternStatement *CASTExternStatement::clone()

{
  return new CASTExternStatement((CASTStringConstant *) buildAllClones(language),
				 (CASTStatement *) buildAllClones(declarations));
}

CASTIterationStatement *CASTIterationStatement::clone()

{
  return new CASTIterationStatement((CASTExpression *) buildAllClones(condition),
				    (CASTStatement *) buildAllClones(block),
				    NULL);
}

CASTDoWhileStatement *CASTDoWhileStatement::clone()

{
  return new CASTDoWhileStatement((CASTExpression *) buildAllClones(condition),
                                  (CASTStatement *) buildAllClones(block));
}

CASTWhileStatement *CASTWhileStatement::clone()

{
  return new CASTWhileStatement((CASTExpression *) buildAllClones(condition),
                                (CASTStatement *) buildAllClones(block));
}

CASTForStatement *CASTForStatement::clone()

{
  return new CASTForStatement((CASTStatement *) buildAllClones(initialization),
			      (CASTExpression *) buildAllClones(condition),
			      (CASTExpression *) buildAllClones(loopExpression),
			      (CASTStatement *) buildAllClones(block));
}

CASTReturnStatement *CASTReturnStatement::clone()

{
  return new CASTReturnStatement((CASTExpression *) buildAllClones(value));
}

CASTLabelStatement *CASTLabelStatement::clone()

{
  return new CASTLabelStatement((CASTIdentifier *) buildAllClones(label),
				(CASTStatement *) buildAllClones(statement));
}

CASTPreprocConditional *CASTPreprocConditional::clone()

{
  return new CASTPreprocConditional((CASTExpression *) buildAllClones(expr),
				    buildAllClones(block));
}

CASTPreprocDefine *CASTPreprocDefine::clone()

{
  return new CASTPreprocDefine(buildAllClones(name),
			       buildAllClones(value));
}

CASTPreprocUndef *CASTPreprocUndef::clone()

{
  return new CASTPreprocUndef(buildAllClones(name));
}

CASTSelectionStatement *CASTSelectionStatement::clone()

{
  return new CASTSelectionStatement((CASTExpression *) buildAllClones(expression),
				    NULL);
}

CASTIfStatement *CASTIfStatement::clone()

{
  return new CASTIfStatement((CASTExpression *) buildAllClones(expression),
			     (CASTStatement *) buildAllClones(positiveBranch),
			     (CASTStatement *) buildAllClones(negativeBranch));
}

CASTSwitchStatement *CASTSwitchStatement::clone()

{
  return new CASTSwitchStatement((CASTExpression *) buildAllClones(condition),
				 (CASTStatement *) buildAllClones(statement));
}

CASTTemplateStatement *CASTTemplateStatement::clone()

{
  return new CASTTemplateStatement((CASTDeclaration *) buildAllClones(parameters),
				   (CASTStatement *) buildAllClones(definition));
}
