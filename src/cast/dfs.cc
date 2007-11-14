#include "cast.h"

#define visitAll(x) do \
  { \
    CASTBase *iterator = peer->x; \
    if (iterator) \
      do { \
           iterator->accept(this); \
           iterator = iterator->next; \
         } while (iterator!=peer->x); \
  } while (0)  

#define enterMajor(a...) do { if (debug_mode & DEBUG_IMPORT) { println("Entering " a); indent(+1); } } while (0)
#define leaveMajor() do { if (debug_mode & DEBUG_IMPORT) { indent(-1); println("Done"); } } while (0)

void CASTDFSVisitor::visit(CASTFile *peer)

{
  enterMajor("file");
  visitAll(definitions);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTAttributeSpecifier *peer)

{
  enterMajor("attribute");
  visitAll(name);
  visitAll(value);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTDeclarator *peer)

{
  enterMajor("declarator: %s", (peer->identifier) ? peer->identifier->identifier : "anonymous");
  visitAll(pointer);
  visitAll(qualifiers);
  visitAll(identifier);
  visitAll(arrayDims);
  visitAll(parameters);
  visitAll(asmName);
  visitAll(subdecl);
  visitAll(initializer);
  visitAll(attributes);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTNestedIdentifier *peer)

{
  enterMajor("nested identifier");
  visitAll(scopeName);
  visitAll(ident);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTUnaryOp *peer)

{
  enterMajor("unary op");
  visitAll(leftExpr);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTCastOp *peer)

{
  enterMajor("cast op");
  visitAll(target);
  visitAll(leftExpr);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTNewOp *peer)

{
  enterMajor("new op");
  visitAll(type);
  visitAll(leftExpr);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTPostfixOp *peer)

{
  enterMajor("postfix op");
  visitAll(leftExpr);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTIndexOp *peer)

{
  enterMajor("index op");
  visitAll(baseExpr);
  visitAll(indexExpr);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTBrackets *peer)

{
  enterMajor("brackets");
  visitAll(expression);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTBinaryOp *peer)

{
  enterMajor("binary op");
  visitAll(leftExpr);
  visitAll(rightExpr);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTConditionalExpression *peer)

{
  enterMajor("conditional expression");
  visitAll(expression);
  visitAll(resultIfTrue);
  visitAll(resultIfFalse);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTArrayInitializer *peer)

{
  enterMajor("array initializer");
  visitAll(elements);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTFunctionOp *peer)

{
  enterMajor("function op");
  visitAll(function);
  visitAll(arguments);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTDeclaration *peer)

{
  enterMajor("declaration");
  visitAll(specifiers);
  visitAll(declarators);
  visitAll(baseInit);
  visitAll(compound);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTTypeSpecifier *peer)

{
  enterMajor("type specifier");
  visitAll(identifier);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTPreprocDefine *peer)

{
  enterMajor("preproc define");
  visitAll(name);
  visitAll(value);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTPreprocConditional *peer)

{
  enterMajor("preproc conditional");
  visitAll(expr);
  visitAll(block);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTDeclarationStatement *peer)

{
  enterMajor("declaration statement");
  visitAll(declaration);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTAggregateSpecifier *peer)

{
  enterMajor("aggregate specifier");
  visitAll(declarations);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTEnumSpecifier *peer)

{
  enterMajor("enum specifier");
  visitAll(elements);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTCompoundStatement *peer)

{
  enterMajor("compound statement");
  visitAll(statements);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTExpressionStatement *peer)

{
  enterMajor("expression statement");
  visitAll(expression);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTReturnStatement *peer)

{
  enterMajor("return statement");
  visitAll(value);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTDoWhileStatement *peer)

{
  enterMajor("do...while statement");
  visitAll(block);
  visitAll(condition);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTWhileStatement *peer)

{
  enterMajor("while statement");
  visitAll(condition);
  visitAll(block);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTForStatement *peer)

{
  enterMajor("for statement");
  visitAll(initialization);
  visitAll(condition);
  visitAll(loopExpression);
  visitAll(block);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTIfStatement *peer)

{
  enterMajor("if statement");
  visitAll(expression);
  visitAll(positiveBranch);
  visitAll(negativeBranch);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTAsmConstraint *peer)

{
  enterMajor("asm constraint");
  visitAll(value);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTAsmStatement *peer)

{
  enterMajor("asm statement");
  visitAll(instructions);
  visitAll(inConstraints);
  visitAll(outConstraints);
  visitAll(clobberConstraints);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTDeclarationExpression *peer)

{
  enterMajor("declaration expression");
  visitAll(contents);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTExternStatement *peer)

{
  enterMajor("extern statement");
  visitAll(declarations);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTConstructorDeclaration *peer)

{
  enterMajor("constructor");
  visitAll(specifiers);
  visitAll(declarators);
  visitAll(compound);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTGotoStatement *peer)

{
  enterMajor("goto statement");
  visitAll(label);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTCaseStatement *peer)

{
  enterMajor("case statement");
  visitAll(discrimValue);
  visitAll(discrimEnd);
  visitAll(statements);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTSwitchStatement *peer)

{
  enterMajor("switch statement");
  visitAll(condition);
  visitAll(statement);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTLabelStatement *peer)

{
  enterMajor("label statement");
  visitAll(label);
  visitAll(statement);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTTemplateStatement *peer)

{
  enterMajor("template statement");
  visitAll(parameters);
  visitAll(definition);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTTemplateTypeDeclaration *peer)

{
  enterMajor("template type declaration");
  visitAll(declarators);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTTemplateTypeIdentifier *peer)

{
  enterMajor("template type identifier");
  visitAll(arguments);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTIndirectTypeSpecifier *peer)

{
  enterMajor("indirect type specifier");
  visitAll(typeExpr);
  leaveMajor();
}

void CASTDFSVisitor::visit(CASTStatementExpression *peer)

{
  enterMajor("statement expression");
  visitAll(contents);
  leaveMajor();
}
