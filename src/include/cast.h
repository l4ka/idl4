#ifndef __cast_h__
#define __cast_h__

#include <assert.h>

#include "globals.h"
#include "base.h"
#include "fe/compiler.h"

#define ANONYMOUS NULL

#define PRECED_SEQUENCE  	0	// ,
#define PRECED_ASSIGNMENT	1	// = *= /= %= += -= <<= >>= &= ^= |=
#define PRECED_CONDITIONAL	2	// ?:
#define PRECED_LOGICAL_OR	3	// ||
#define PRECED_LOGICAL_AND	4	// &&
#define PRECED_INCLUSIVE_OR	5	// |
#define PRECED_EXCLUSIVE_OR	6	// ^
#define PRECED_AND		7	// &
#define PRECED_MIN_MAX		8	// >? <?
#define PRECED_EQUALITY		9	// == !=
#define PRECED_RELATIONAL	10	// < > <= >=
#define PRECED_SHIFT		11	// << >>
#define PRECED_ADDITIVE		12	// + -
#define PRECED_MULTIPLICATIVE	13	// * / %
#define PRECED_CAST		14	// (type)
#define PRECED_UNARY		15	// ++c --c  sizeof & * +c -c ~ !
#define PRECED_POSTFIX		16	// [ind] (arg) a.b a->b a++ a-- 
#define PRECED_PRIMARY		17	// a 1 ()

enum CASTVisibility { VIS_PRIVATE, VIS_PROTECTED, VIS_PUBLIC, VIS_DONTCARE };

class CASTVisitor;

class CASTContext : public CContext

{
public:
  CASTContext(const char *file, const char *line, int lineNo, int pos) : CContext(file, line, lineNo, pos)
    { };
};

class CASTBase : public CBase

{
protected:
  static bool cxxMode;

public:
  CASTBase *prev, *next;
  CASTContext *context;

  CASTBase(CASTContext *context = NULL) 
    { this->prev = this; this->next = this; this->context = context; };
  void add(CASTBase *newElement);
  virtual void dump() { assert(false); };
  virtual bool isEndOfList() { return false; };
  virtual void write() { assert(false); };
  void writeAll(const char *separator = NULL);
  virtual CASTBase *clone();
  virtual CASTBase *cloneAll();
  virtual void accept(CASTVisitor *worker);
  void acceptAll(CASTVisitor *worker);
  CASTBase *remove();
  static void setCXXMode(bool state)
    { cxxMode = state; }
};

class CASTFile : public CASTBase

{
public:
  const char *name;
  CASTBase *definitions;

  CASTFile(const char *name, CASTBase *definitions) : CASTBase() 
    { this->definitions = definitions; this->name = name; };
  virtual void write();
  virtual void accept(CASTVisitor *worker);
  void open();
  void close();
};

class CASTExpression;
class CASTIdentifier;

class CASTPointer : public CASTBase

{
public:
  CASTPointer(int level = 1) : CASTBase() { if (level>1) add(new CASTPointer(level-1)); };
  virtual void write();
  virtual CASTPointer *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTRef : public CASTPointer

{
public:
  CASTRef(int level = 1) : CASTPointer(1) { if (level>1) add(new CASTRef(level-1)); };
  virtual void write();
  virtual CASTRef *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTExpression : public CASTBase

{
public:
  CASTExpression() : CASTBase() 
    { };
  virtual int getPrecedenceLevel() { assert(false); return 0; };
  virtual CASTExpression *clone();
  bool equals(CASTExpression *expr);
  virtual void accept(CASTVisitor *worker);
  virtual bool isConstant() { return false; };
};

class CASTLabeledExpression : public CASTExpression

{
  CASTIdentifier *name;
  CASTExpression *value;
  
public:
  CASTLabeledExpression(CASTIdentifier *name, CASTExpression *value) : CASTExpression()
    { this->name = name; this->value = value; };
  virtual int getPrecedenceLevel() { return value->getPrecedenceLevel(); };
  virtual void accept(CASTVisitor *worker);
  virtual CASTLabeledExpression *clone();
  virtual void write();
};

class CASTDeclaration;
class CASTCompoundStatement;

class CASTDeclarationSpecifier;
class CASTAttributeSpecifier;
class CASTStringConstant;

class CASTDeclarator : public CASTBase

{
public:
  CASTPointer *pointer;
  CASTDeclarationSpecifier *qualifiers;
  CASTIdentifier *identifier;
  CASTExpression *arrayDims;
  CASTDeclaration *parameters;
  CASTDeclarator *subdecl;
  CASTExpression *initializer;
  CASTAttributeSpecifier *attributes;
  CASTStringConstant *asmName;
  CASTExpression *bitSize;
  
  CASTDeclarator(CASTIdentifier *identifier, CASTPointer *pointer = NULL, CASTExpression *arrayDims = NULL, CASTExpression *initializer = NULL, CASTAttributeSpecifier *attributes = NULL) : CASTBase() 
    { this->identifier = identifier; this->subdecl = NULL; this->arrayDims = arrayDims; this->pointer = pointer; this->parameters = NULL; this->initializer = initializer; this->attributes = attributes; this->qualifiers = NULL; this->bitSize = NULL; this->asmName = NULL; };
  CASTDeclarator(CASTDeclarator *subdecl, CASTPointer *pointer = NULL, CASTExpression *arrayDims = NULL, CASTDeclarationSpecifier *qualifiers = NULL) : CASTBase() 
    { this->identifier = NULL; this->subdecl = subdecl; this->arrayDims = arrayDims; this->pointer = pointer; this->parameters = NULL; this->initializer = NULL; this->attributes = NULL; this->qualifiers = qualifiers; this->bitSize = NULL; this->asmName = NULL; };
  CASTDeclarator(CASTIdentifier *identifier, CASTDeclaration *parameters, CASTPointer *pointer = NULL, CASTDeclarator *subdecl = NULL, CASTExpression *arrayDims = NULL, CASTAttributeSpecifier *attributes = NULL, CASTDeclarationSpecifier *qualifiers = NULL, CASTStringConstant *asmName = NULL)
    { this->identifier = identifier; this->subdecl = NULL; this->arrayDims = NULL; this->pointer = pointer, this->parameters = parameters; this->initializer = NULL; this->attributes = attributes; this->qualifiers = qualifiers; this->bitSize = NULL; this->asmName = asmName; };
  void addIndir(int levels);
  void addRef(int levels);
  void addArrayDim(CASTExpression *dim)
    { if (arrayDims) arrayDims->add(dim); else arrayDims = dim; };
  void setParameters(CASTDeclaration *parameters)
    { this->parameters = parameters; };
  void setInitializer(CASTExpression *initializer)
    { this->initializer = initializer; };
  void setBitSize(CASTExpression *bitSize)
    { this->bitSize = bitSize; };
  void setAttributes(CASTAttributeSpecifier *attributes)
    { this->attributes = attributes; };
  void setQualifiers(CASTDeclarationSpecifier *qualifiers)
    { this->qualifiers = qualifiers; };
  void setAsmName(CASTStringConstant *asmName)
    { this->asmName = asmName; };
  CASTIdentifier *getIdentifier()
    { if (subdecl) return subdecl->getIdentifier(); else return identifier; };
  virtual void write();
  virtual CASTDeclarator *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTIdentifier : public CASTExpression

{
public:
  const char *identifier;
  
  CASTIdentifier(const char *identifier) : CASTExpression() 
    { this->identifier = identifier; };
  virtual void write();
  virtual void addPrefix(const char *str);
  virtual void addPostfix(const char *str);
  virtual void capitalize();
  virtual int getPrecedenceLevel() { return PRECED_PRIMARY; };
  virtual const char *getIdentifier() { return identifier; };
  virtual CASTIdentifier *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTTemplateTypeIdentifier : public CASTIdentifier

{
public:
  CASTExpression *arguments;

  CASTTemplateTypeIdentifier(const char *identifier, CASTExpression *arguments) : CASTIdentifier(identifier)
    { this->arguments = arguments; };
  virtual void write();
  virtual CASTTemplateTypeIdentifier *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTOperatorIdentifier : public CASTIdentifier

{
public:
  CASTOperatorIdentifier(const char *identifier) : CASTIdentifier(identifier)
    { };
  virtual void write();
  virtual CASTOperatorIdentifier *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTEllipsis : public CASTDeclarator

{
public:
  CASTEllipsis() : CASTDeclarator(new CASTIdentifier("..."))
    {  };
  virtual void accept(CASTVisitor *worker);
  virtual CASTEllipsis *clone()
    { return new CASTEllipsis(); }
};

class CASTNestedIdentifier : public CASTIdentifier

{
public:
  CASTIdentifier *scopeName, *ident;

  CASTNestedIdentifier(CASTIdentifier *scopeName, CASTIdentifier *ident) : CASTIdentifier(NULL)
    { this->scopeName = scopeName; this->ident = ident; };
  virtual void write();
  virtual CASTNestedIdentifier *clone();
  virtual void accept(CASTVisitor *worker);
  virtual const char *getIdentifier()
    { return mprintf("%s%s%s",
		     (scopeName)?scopeName->getIdentifier():"",
		     ((scopeName)&&(ident))?"::":"",
		     (ident)?ident->getIdentifier():""); }
		     
};

class CASTUnaryOp : public CASTExpression

{
public:
  CASTExpression *leftExpr;
  const char *op;
  
  CASTUnaryOp(const char *op, CASTExpression *expr) : CASTExpression()
    { this->op = op; this->leftExpr = expr; };
  virtual void write();
  virtual int getPrecedenceLevel() { return PRECED_UNARY; };
  virtual CASTUnaryOp *clone();
  virtual void accept(CASTVisitor *worker);
};

enum CASTCastType { CASTStandardCast, CASTFunctionalCast, CASTStaticCast, CASTDynamicCast, CASTReinterpretCast };

class CASTCastOp : public CASTUnaryOp

{
public:
  CASTDeclaration *target;
  CASTCastType type;
  
  CASTCastOp(CASTDeclaration *target, CASTExpression *expr, CASTCastType type) : CASTUnaryOp("CAST", expr)
    { this->target = target; this->type = type; };
  virtual void write();
  virtual int getPrecedenceLevel() { return PRECED_CAST; };
  virtual CASTCastOp *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTNewOp : public CASTUnaryOp

{
public:
  CASTDeclarationSpecifier *type;
  CASTExpression *placementArgs;
  
  CASTNewOp(CASTDeclarationSpecifier *type, CASTExpression *initializers, CASTExpression *placementArgs = NULL) : CASTUnaryOp("NEW", initializers)
    { this->type = type; this->placementArgs = placementArgs; };
  virtual void write();
  virtual CASTNewOp *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTPostfixOp : public CASTUnaryOp

{
public:
  CASTPostfixOp(const char *op, CASTExpression *expr) : CASTUnaryOp(op, expr) 
    { };
  virtual void write();
  virtual int getPrecedenceLevel() { return PRECED_POSTFIX; };
  virtual CASTPostfixOp *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTIndexOp : public CASTExpression

{
public:
  CASTExpression *baseExpr, *indexExpr;
  
  CASTIndexOp(CASTExpression *baseExpr, CASTExpression *indexExpr) : CASTExpression()
    { this->baseExpr = baseExpr; this->indexExpr = indexExpr; };
  virtual void write();
  virtual int getPrecedenceLevel() { return PRECED_POSTFIX; };
  virtual CASTIndexOp *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTBrackets : public CASTExpression

{
public:
  CASTExpression *expression;
  
  CASTBrackets(CASTExpression *expression) : CASTExpression() 
    { this->expression = expression; };
  virtual void write();
  virtual int getPrecedenceLevel() { return PRECED_PRIMARY; };
  virtual CASTBrackets *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTBinaryOp : public CASTUnaryOp

{
public:
  CASTExpression *rightExpr;
  
  CASTBinaryOp(const char *op, CASTExpression *leftExpr, CASTExpression *rightExpr) : CASTUnaryOp(op, leftExpr)
    { this->rightExpr = rightExpr; };
  virtual void write();
  virtual int getPrecedenceLevel();
  virtual CASTBinaryOp *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTConditionalExpression : public CASTExpression

{
public:
  CASTExpression *expression, *resultIfTrue, *resultIfFalse;
  
  CASTConditionalExpression(CASTExpression *expression, CASTExpression *resultIfTrue, CASTExpression *resultIfFalse) : CASTExpression()
    { this->expression = expression; this->resultIfTrue = resultIfTrue; this->resultIfFalse = resultIfFalse; };
  virtual void write();
  virtual CASTConditionalExpression *clone();
  virtual int getPrecedenceLevel() { return PRECED_CONDITIONAL; };
  virtual void accept(CASTVisitor *worker);
};

class CASTArrayInitializer : public CASTExpression

{
public:
  CASTExpression *elements;
  
  CASTArrayInitializer(CASTExpression *elements) : CASTExpression()
    { this->elements = elements; };
  virtual void write();
  CASTArrayInitializer *clone();
  virtual int getPrecedenceLevel() { return PRECED_PRIMARY; };
  virtual void accept(CASTVisitor *worker);
};

class CASTFunctionOp : public CASTExpression

{
public:
  CASTExpression *function;
  CASTExpression *arguments;
  
  CASTFunctionOp(CASTExpression *function, CASTExpression *arguments = NULL) : CASTExpression()
    { this->function = function; this->arguments = arguments; };
  virtual void write();
  virtual int getPrecedenceLevel() { return PRECED_POSTFIX; };
  virtual CASTFunctionOp *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTConstant : public CASTExpression

{
public:
  CASTConstant() : CASTExpression() 
    { };
  virtual int getPrecedenceLevel() { return PRECED_PRIMARY; };
  virtual void accept(CASTVisitor *worker);
};

class CASTIntegerConstant : public CASTConstant

{
public:
  unsigned long long value;
  bool isUnsigned, isLong, isLongLong;
  
  CASTIntegerConstant(unsigned long long value, bool isUnsigned = false, bool isLong = false, bool isLongLong = false) : CASTConstant() 
    { this->value = value; this->isUnsigned = isUnsigned; this->isLong = isLong; this->isLongLong = isLongLong; };
  virtual void write();
  virtual CASTIntegerConstant *clone();
  virtual void accept(CASTVisitor *worker);
  virtual bool isConstant() { return true; };
};

class CASTHexConstant : public CASTIntegerConstant

{
public:
  CASTHexConstant(unsigned long long value, bool isUnsigned = false, bool isLong = false, bool isLongLong = false) : CASTIntegerConstant(value, isUnsigned, isLong, isLongLong) 
    { };
  virtual void write();
  virtual CASTHexConstant *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTFloatConstant : public CASTConstant

{
public:
  long double value;
  
  CASTFloatConstant(long double value) : CASTConstant() { this->value = value; };
  virtual void write();
  virtual CASTFloatConstant *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTStringConstant : public CASTConstant

{
public:
  bool isWide;
  const char *value;
  
  CASTStringConstant(bool isWide, const char *value) : CASTConstant() 
    { this->isWide = isWide; this->value = value; };
  virtual void write();
  CASTStringConstant *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTCharConstant : public CASTIntegerConstant

{
public:
  bool isWide;
  
  CASTCharConstant(bool isWide, int value) : CASTIntegerConstant(value) 
    { this->isWide = isWide; };
  virtual void write();
  virtual CASTCharConstant *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTDefaultConstant : public CASTConstant
  
{
public:
  CASTDefaultConstant() : CASTConstant() 
    { };
  virtual void write();
  virtual CASTDefaultConstant *clone()
    { return new CASTDefaultConstant(); }
  virtual void accept(CASTVisitor *worker);
};

class CASTEmptyConstant : public CASTConstant

{
public:
  CASTEmptyConstant() : CASTConstant() 
    { };
  virtual void write();
  virtual CASTEmptyConstant *clone()
    { return new CASTEmptyConstant(); }
  virtual void accept(CASTVisitor *worker);
};

class CASTDeclarationSpecifier : public CASTBase

{
public:
  CASTDeclarationSpecifier() : CASTBase() 
    { };
  virtual CASTDeclarationSpecifier *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTAttributeSpecifier : public CASTDeclarationSpecifier

{
public:
  CASTIdentifier *name;
  CASTExpression *value;
  
  CASTAttributeSpecifier(CASTIdentifier *name, CASTExpression *value = NULL) : CASTDeclarationSpecifier()
    { this->name = name; this->value = value; };
  virtual void write();
  virtual CASTAttributeSpecifier *clone();
  virtual void accept(CASTVisitor *worker);
};  

class CASTStorageClassSpecifier : public CASTDeclarationSpecifier

{
public:
  const char *keyword;
  
  CASTStorageClassSpecifier(const char *keyword) : CASTDeclarationSpecifier()
    { this->keyword = keyword; }
  virtual void write();
  virtual CASTStorageClassSpecifier *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTConstOrVolatileSpecifier : public CASTDeclarationSpecifier

{
public:
  const char *keyword;
  
  CASTConstOrVolatileSpecifier(const char *keyword) : CASTDeclarationSpecifier()
    { this->keyword = keyword; }
  virtual void write();
  virtual CASTConstOrVolatileSpecifier *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTTypeSpecifier : public CASTDeclarationSpecifier

{
public:
  CASTIdentifier *identifier;
  
  CASTTypeSpecifier(CASTIdentifier *identifier) : CASTDeclarationSpecifier()
    { this->identifier = identifier; };
  CASTTypeSpecifier(const char *identifier) : CASTDeclarationSpecifier()
    { this->identifier = new CASTIdentifier(identifier); };
  virtual void write();
  virtual CASTTypeSpecifier *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTIndirectTypeSpecifier : public CASTDeclarationSpecifier

{
public:
  const char *operatorName;
  CASTExpression *typeExpr;
  
  CASTIndirectTypeSpecifier(const char *operatorName, CASTExpression *typeExpr) : CASTDeclarationSpecifier()
    { this->typeExpr = typeExpr; this->operatorName = operatorName; };
  virtual void write();
  virtual CASTIndirectTypeSpecifier *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTDeclaration : public CASTBase

{
public:
  CASTDeclarationSpecifier *specifiers;
  CASTDeclarator *declarators;
  CASTExpression *baseInit;
  CASTCompoundStatement *compound;
  
  CASTDeclaration(CASTDeclarationSpecifier *specifiers, CASTDeclarator *declarators, CASTCompoundStatement *compound = NULL, CASTExpression *baseInit = NULL) : CASTBase() 
    { this->specifiers = specifiers; this->declarators = declarators; this->compound = compound; this->baseInit = baseInit; };
  virtual void addSpecifier(CASTDeclarationSpecifier *newSpecifier);
  virtual void setCompound(CASTCompoundStatement *compound)
    { this->compound = compound; };
  virtual void setBaseInit(CASTExpression *baseInit)
    { this->baseInit = baseInit; };
  virtual void stripSpecifiers() { this->specifiers = NULL; };
  virtual CASTDeclarator *getDeclarators()
    { return declarators; };
  virtual void write();
  virtual CASTDeclaration *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTEmptyDeclaration : public CASTDeclaration

{
public:
  CASTEmptyDeclaration() : CASTDeclaration(NULL, NULL) 
    { };
  virtual void write();
  virtual void accept(CASTVisitor *worker);
};

class CASTTemplateTypeDeclaration : public CASTDeclaration

{
public:
  CASTTemplateTypeDeclaration(CASTDeclarator *declarator) : CASTDeclaration(NULL, declarator)
    { };
  virtual void write();
  virtual CASTTemplateTypeDeclaration *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTStatement : public CASTBase

{
public:
  CASTStatement(CASTContext *context = NULL) : CASTBase(context) 
    { };
  virtual CASTStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTTemplateStatement : public CASTStatement

{
public:
  CASTStatement *definition;
  CASTDeclaration *parameters;
  
  CASTTemplateStatement(CASTDeclaration *parameters = NULL, CASTStatement *definition = NULL, CASTContext *context = NULL) : CASTStatement(context)
    { this->parameters = parameters; this->definition = definition; };
  virtual void setDefinition(CASTStatement *definition)
    { this->definition = definition; };
  virtual void write();
  virtual CASTTemplateStatement *clone();
  virtual void accept(CASTVisitor *worker);
};
  
class CASTSwitchStatement : public CASTStatement

{
public:
  CASTExpression *condition;
  CASTStatement *statement;
  
  CASTSwitchStatement(CASTExpression *condition, CASTStatement *statement, CASTContext *context = NULL) : CASTStatement(context)
    { this->condition = condition; this->statement = statement; };
  virtual void write();
  virtual CASTSwitchStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTCaseStatement : public CASTStatement

{
public:
  CASTExpression *discrimValue, *discrimEnd;
  CASTStatement *statements;
  
  CASTCaseStatement(CASTStatement *statements, CASTExpression *discrimValue, CASTExpression *discrimEnd = NULL, CASTContext *context = NULL) : CASTStatement(context)
    { this->discrimValue = discrimValue; this->statements = statements; this->discrimEnd = discrimEnd; };
  virtual void write();
  virtual CASTCaseStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTLabelStatement : public CASTStatement

{
public:
  CASTIdentifier *label;
  CASTStatement *statement;
  
  CASTLabelStatement(CASTIdentifier *label, CASTStatement *statement, CASTContext *context = NULL) : CASTStatement(context)
    { this->label = label; this->statement = statement; };
  virtual void write();
  virtual CASTLabelStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTPreprocDefine : public CASTStatement

{
public:
  CASTBase *name;
  CASTBase *value;
  
  CASTPreprocDefine(CASTBase *name, CASTBase *value = NULL) : CASTStatement()
    { this->name = name; this->value = value; };
  virtual void write();
  virtual CASTPreprocDefine *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTPreprocUndef : public CASTStatement

{
public:
  CASTBase *name;
  
  CASTPreprocUndef(CASTBase *name) : CASTStatement()
    { this->name = name; };
  virtual void write();
  virtual CASTPreprocUndef *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTPreprocConditional : public CASTStatement

{
public:
  CASTExpression *expr;
  CASTBase *block;
  
  CASTPreprocConditional(CASTExpression *expr, CASTBase *block) : CASTStatement()
    { this->expr = expr; this->block = block; };
  virtual void write();
  virtual CASTPreprocConditional *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTPreprocInclude : public CASTStatement

{
public:
  CASTIdentifier *name;
  
  CASTPreprocInclude(CASTIdentifier *name) : CASTStatement()
    { this->name = name; };
  virtual void write();
  virtual CASTPreprocInclude *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTPreprocError : public CASTStatement

{
public:
  CASTIdentifier *message;
  
  CASTPreprocError(CASTIdentifier *message) : CASTStatement()
    { this->message = message; };
  virtual void write();
  virtual CASTPreprocError *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTComment : public CASTStatement

{
public:
  const char *text;

  CASTComment(const char *text) : CASTStatement() { this->text = text; };
  virtual void write();
  virtual CASTComment *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTBlockComment : public CASTComment

{
public:
  CASTBlockComment(const char *text) : CASTComment(text) 
    { };
  virtual void write();
  virtual CASTBlockComment *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTSpacer : public CASTStatement

{
public:
  CASTSpacer() : CASTStatement() 
    { };
  virtual void write();
  virtual CASTSpacer *clone()
    { return new CASTSpacer(); }
  virtual void accept(CASTVisitor *worker);
};

class CASTDeclarationStatement : public CASTStatement

{
public:
  CASTDeclaration *declaration;
  CASTVisibility visibility;
 
  CASTDeclarationStatement(CASTDeclaration *declaration, CASTVisibility visibility = VIS_PRIVATE, CASTContext *context = NULL) : CASTStatement(context)
    { this->declaration = declaration; this->visibility = visibility; };
  virtual void write();
  virtual CASTDeclarationStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTBaseClass : public CASTBase

{
public:
  CASTStorageClassSpecifier *specifiers;
  CASTIdentifier *identifier;
  CASTVisibility visibility;
  
  CASTBaseClass(CASTIdentifier *identifier, CASTVisibility visibility, CASTStorageClassSpecifier *specifiers = NULL) : CASTBase()
    { this->identifier = identifier; this->visibility = visibility; this->specifiers = specifiers; };
  void setSpecifiers(CASTStorageClassSpecifier *specifiers)
    { this->specifiers = specifiers; };
  virtual void write();
  virtual CASTBaseClass *clone();
  virtual void accept(CASTVisitor *worker);
};
  
class CASTAggregateSpecifier : public CASTTypeSpecifier

{
public:
  CASTStatement *declarations;
  CASTAttributeSpecifier *attributes;
  CASTBaseClass *parents;
  const char *type;
  
  CASTAggregateSpecifier(const char *type, CASTIdentifier *identifier = NULL, CASTStatement *declarations = NULL) : CASTTypeSpecifier(identifier)
    { this->type = type; this->declarations = declarations; this->parents = NULL; this->attributes = NULL; };
  void setDeclarations(CASTStatement *declarations)
    { this->declarations = declarations; };
  void setIdentifier(CASTIdentifier *identifier)
    { this->identifier = identifier; };
  void setParents(CASTBaseClass *parents)
    { this->parents = parents; };
  void setAttributes(CASTAttributeSpecifier *attributes)
    { this->attributes = attributes; };
  CASTIdentifier *getIdentifier()
    { return identifier; };
  virtual void write();
  virtual CASTAggregateSpecifier *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTEnumSpecifier : public CASTTypeSpecifier

{
public:
  CASTDeclarator *elements;
  
  CASTEnumSpecifier(CASTIdentifier *identifier, CASTDeclarator *elements) : CASTTypeSpecifier(identifier)
    { this->elements = elements; };
  virtual void write();
  virtual CASTEnumSpecifier *clone();
  virtual void accept(CASTVisitor *worker);
};
 
class CASTCompoundStatement : public CASTStatement

{
public:
  CASTStatement *statements;
  
  CASTCompoundStatement(CASTStatement *statements, CASTContext *context = NULL) : CASTStatement(context)
    { this->statements = statements; };
  virtual void write();
  virtual CASTCompoundStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTExpressionStatement : public CASTStatement

{
public:
  CASTExpression *expression;
  
  CASTExpressionStatement(CASTExpression *expression, CASTContext *context = NULL) : CASTStatement(context)
    { this->expression = expression; };
  virtual void write();  
  virtual CASTExpressionStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTJumpStatement : public CASTStatement

{
public:
  CASTJumpStatement(CASTContext *context) : CASTStatement(context) 
    { };
  virtual CASTJumpStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTReturnStatement : public CASTJumpStatement

{
public:
  CASTExpression *value;

  CASTReturnStatement(CASTExpression *value = NULL, CASTContext *context = NULL) : CASTJumpStatement(context)
    { this->value = value; };
  virtual void write();
  virtual CASTReturnStatement *clone();
  virtual void accept(CASTVisitor *worker);
};  

class CASTBreakStatement : public CASTJumpStatement

{
public:
  CASTBreakStatement(CASTContext *context = NULL) : CASTJumpStatement(context) 
    { };
  virtual void write();
  virtual CASTBreakStatement *clone();
  virtual void accept(CASTVisitor *worker);
};  

class CASTContinueStatement : public CASTJumpStatement

{
public:
  CASTContinueStatement(CASTContext *context = NULL) : CASTJumpStatement(context) 
    { };
  virtual void write();
  virtual CASTContinueStatement *clone();
  virtual void accept(CASTVisitor *worker);
};  

class CASTGotoStatement : public CASTJumpStatement

{
public:
  CASTIdentifier *label;
  
  CASTGotoStatement(CASTIdentifier *label, CASTContext *context = NULL) : CASTJumpStatement(context) 
    { this->label = label; };
  virtual void write();
  virtual CASTGotoStatement *clone();
  virtual void accept(CASTVisitor *worker);
};  

// CASTJumpStatement : CASTStatement		goto continue break return
// CASTLabelledStatement : CASTStatement	label:
// CASTSelectionStatement : CASTStatement	if...else switch
// CASTIterationStatement : CASTStatement	while for do

class CASTIterationStatement : public CASTStatement

{
public:
  CASTStatement *block;
  CASTExpression *condition;
  
  CASTIterationStatement(CASTExpression *condition, CASTStatement *block, CASTContext *context) : CASTStatement(context) 
    { this->block = block; this->condition = condition; };
  virtual void accept(CASTVisitor *worker);
  virtual CASTIterationStatement *clone();
};

class CASTDoWhileStatement : public CASTIterationStatement

{
public:
  CASTDoWhileStatement(CASTExpression *condition, CASTStatement *block, CASTContext *context = NULL) : CASTIterationStatement(condition, block, context) 
    { };
  virtual void write();
  virtual CASTDoWhileStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTWhileStatement : public CASTIterationStatement

{
public:
  CASTWhileStatement(CASTExpression *condition, CASTStatement *block = NULL, CASTContext *context = NULL) : CASTIterationStatement(condition, block, context) 
    { };
  virtual void write();
  virtual CASTWhileStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTForStatement : public CASTIterationStatement

{
public:
  CASTStatement *initialization;
  CASTExpression *loopExpression;

  CASTForStatement(CASTStatement *initialization, CASTExpression *condition, CASTExpression *loopExpression, CASTStatement *block, CASTContext *context = NULL) : CASTIterationStatement(condition, block, context)
    { this->initialization = initialization; this->loopExpression = loopExpression; };
  virtual void write();
  virtual CASTForStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTSelectionStatement : public CASTStatement

{
public:
  CASTExpression *expression;
  
  CASTSelectionStatement(CASTExpression *expression, CASTContext *context) : CASTStatement(context) 
    { this->expression = expression; };
  virtual CASTSelectionStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTIfStatement : public CASTSelectionStatement

{
public:
  CASTStatement *positiveBranch, *negativeBranch;

  CASTIfStatement(CASTExpression *expression, CASTStatement *positiveBranch, CASTStatement *negativeBranch = NULL, CASTContext *context = NULL) : CASTSelectionStatement(expression, context) 
    { this->positiveBranch = positiveBranch; this->negativeBranch = negativeBranch; };
  void setNegativeBranch(CASTStatement *negativeBranch)
    { this->negativeBranch = negativeBranch; };
  virtual void write();
  virtual CASTIfStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTAsmInstruction : public CASTBase

{
public:
  const char *instruction;
  
  CASTAsmInstruction(const char *instruction) : CASTBase()  
    { this->instruction = instruction; };
  virtual void write();
  virtual void accept(CASTVisitor *worker);
  virtual CASTAsmInstruction *clone()
    { return new CASTAsmInstruction(instruction); }
};

class CASTAsmConstraint : public CASTBase

{
public:
  const char *name;
  CASTExpression *value;
  
  CASTAsmConstraint(const char *name, CASTExpression *value = NULL) : CASTBase()
    { this->value = value; this->name = name; };
  virtual void write();
  virtual CASTAsmConstraint *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTAsmStatement : public CASTStatement

{
public:
  CASTConstOrVolatileSpecifier *specifier;
  CASTAsmConstraint *inConstraints, *outConstraints;
  CASTAsmInstruction *instructions;
  CASTBase *clobberConstraints;

  CASTAsmStatement(CASTAsmInstruction *instructions, CASTAsmConstraint *inConstraints, CASTAsmConstraint *outConstraints, CASTBase *clobberConstraints, CASTConstOrVolatileSpecifier *specifier, CASTContext *context = NULL) : CASTStatement(context)
    { this->inConstraints = inConstraints; this->outConstraints = outConstraints; this->clobberConstraints = clobberConstraints; this->instructions = instructions; this->specifier = specifier; };
  virtual void write();
  virtual CASTAsmStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTDeclarationExpression : public CASTExpression

{
public:
  CASTDeclaration *contents;
  
  CASTDeclarationExpression(CASTDeclaration *contents) : CASTExpression()
    { this->contents = contents; };
  virtual void write();
  virtual int getPrecedenceLevel() { return PRECED_PRIMARY; };
  virtual CASTDeclarationExpression *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTStatementExpression : public CASTExpression

{
public:
  CASTStatement *contents;
  
  CASTStatementExpression(CASTStatement *contents) : CASTExpression()
    { this->contents = contents; };
  virtual void write();
  virtual CASTStatementExpression *clone();
  virtual int getPrecedenceLevel() { return PRECED_PRIMARY; };
  virtual void accept(CASTVisitor *worker);
};

class CASTExternStatement : public CASTStatement

{
public:
  CASTStringConstant *language;
  CASTStatement *declarations;
 
  CASTExternStatement(CASTStringConstant *language, CASTStatement *declarations = NULL, CASTContext *context = NULL) : CASTStatement(context)
    { this->language = language; this->declarations = declarations; };
  void setDeclarations(CASTStatement *declarations)
    { this->declarations = declarations; };
  virtual void write();  
  virtual CASTExternStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTEmptyStatement : public CASTStatement

{
public:
  CASTEmptyStatement(CASTContext *context = NULL) : CASTStatement(context)
    { };
  virtual void write();
  virtual CASTEmptyStatement *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTBooleanConstant : public CASTConstant

{
public:
  bool value;
  
  CASTBooleanConstant(bool value) : CASTConstant() 
    { this->value = value; };
  virtual void write();
  virtual CASTBooleanConstant *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTBuiltinConstant : public CASTConstant

{
public:
  const char *name;

  CASTBuiltinConstant(const char *name) : CASTConstant() 
    { this->name = name; };
  virtual void write();
  virtual CASTBuiltinConstant *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTConstructorDeclaration : public CASTDeclaration

{
public:
  CASTConstructorDeclaration(CASTDeclarationSpecifier *specifiers, CASTDeclarator *declarators, CASTCompoundStatement *compound = NULL) : CASTDeclaration(specifiers, declarators, compound) 
    { };
  virtual CASTConstructorDeclaration *clone();
  virtual void accept(CASTVisitor *worker);
};

class CASTVisitor : public CBase

{
public:
  virtual void visit(CASTBase *peer) { assert(false); };
  virtual void visit(CASTFile *peer) { assert(false); };
  virtual void visit(CASTPointer *peer) { assert(false); };
  virtual void visit(CASTExpression *peer) { assert(false); };
  virtual void visit(CASTAttributeSpecifier *peer) { assert(false); };
  virtual void visit(CASTDeclarator *peer) { assert(false); };
  virtual void visit(CASTIdentifier *peer) { assert(false); };
  virtual void visit(CASTEllipsis *peer) { assert(false); };
  virtual void visit(CASTNestedIdentifier *peer) { assert(false); };
  virtual void visit(CASTUnaryOp *peer) { assert(false); };
  virtual void visit(CASTCastOp *peer) { assert(false); };
  virtual void visit(CASTPostfixOp *peer) { assert(false); };
  virtual void visit(CASTNewOp *peer) { assert(false); };
  virtual void visit(CASTIndexOp *peer) { assert(false); };
  virtual void visit(CASTBrackets *peer) { assert(false); };
  virtual void visit(CASTBinaryOp *peer) { assert(false); };
  virtual void visit(CASTConditionalExpression *peer) { assert(false); };
  virtual void visit(CASTArrayInitializer *peer) { assert(false); };
  virtual void visit(CASTFunctionOp *peer) { assert(false); };
  virtual void visit(CASTConstant *peer) { assert(false); };
  virtual void visit(CASTIntegerConstant *peer) { assert(false); };
  virtual void visit(CASTHexConstant *peer) { assert(false); };
  virtual void visit(CASTFloatConstant *peer) { assert(false); };
  virtual void visit(CASTStringConstant *peer) { assert(false); };
  virtual void visit(CASTCharConstant *peer) { assert(false); };
  virtual void visit(CASTDefaultConstant *peer) { assert(false); };
  virtual void visit(CASTStorageClassSpecifier *peer) { assert(false); };
  virtual void visit(CASTConstOrVolatileSpecifier *peer) { assert(false); };
  virtual void visit(CASTTypeSpecifier *peer) { assert(false); };
  virtual void visit(CASTDeclaration *peer) { assert(false); };
  virtual void visit(CASTEmptyDeclaration *peer) { assert(false); };
  virtual void visit(CASTStatement *peer) { assert(false); };
  virtual void visit(CASTPreprocDefine *peer) { assert(false); };
  virtual void visit(CASTPreprocConditional *peer) { assert(false); };
  virtual void visit(CASTPreprocInclude *peer) { assert(false); };
  virtual void visit(CASTPreprocUndef *peer) { assert(false); };
  virtual void visit(CASTPreprocError *peer) { assert(false); };
  virtual void visit(CASTComment *peer) { assert(false); };
  virtual void visit(CASTBlockComment *peer) { assert(false); };
  virtual void visit(CASTSpacer *peer) { assert(false); };
  virtual void visit(CASTDeclarationStatement *peer) { assert(false); };
  virtual void visit(CASTBaseClass *peer) { assert(false); };
  virtual void visit(CASTAggregateSpecifier *peer) { assert(false); };
  virtual void visit(CASTEnumSpecifier *peer) { assert(false); };
  virtual void visit(CASTCompoundStatement *peer) { assert(false); };
  virtual void visit(CASTExpressionStatement *peer) { assert(false); };
  virtual void visit(CASTJumpStatement *peer) { assert(false); };
  virtual void visit(CASTReturnStatement *peer) { assert(false); };
  virtual void visit(CASTBreakStatement *peer) { assert(false); };
  virtual void visit(CASTIterationStatement *peer) { assert(false); };
  virtual void visit(CASTDoWhileStatement *peer) { assert(false); };
  virtual void visit(CASTWhileStatement *peer) { assert(false); };
  virtual void visit(CASTForStatement *peer) { assert(false); };
  virtual void visit(CASTSelectionStatement *peer) { assert(false); };
  virtual void visit(CASTIfStatement *peer) { assert(false); };
  virtual void visit(CASTAsmInstruction *peer) { assert(false); };
  virtual void visit(CASTAsmConstraint *peer) { assert(false); };
  virtual void visit(CASTAsmStatement *peer) { assert(false); };
  virtual void visit(CASTDeclarationExpression *peer) { assert(false); };
  virtual void visit(CASTExternStatement *peer) { assert(false); };
  virtual void visit(CASTEmptyStatement *peer) { assert(false); };
  virtual void visit(CASTBooleanConstant *peer) { assert(false); };
  virtual void visit(CASTBuiltinConstant *peer) { assert(false); };
  virtual void visit(CASTConstructorDeclaration *peer) { assert(false); };
  virtual void visit(CASTGotoStatement *peer) { assert(false); };
  virtual void visit(CASTCaseStatement *peer) { assert(false); };
  virtual void visit(CASTSwitchStatement *peer) { assert(false); };
  virtual void visit(CASTLabelStatement *peer) { assert(false); };
  virtual void visit(CASTTemplateStatement *peer) { assert(false); };
  virtual void visit(CASTTemplateTypeDeclaration *peer) { assert(false); };
  virtual void visit(CASTTemplateTypeIdentifier *peer) { assert(false); };
  virtual void visit(CASTOperatorIdentifier *peer) { assert(false); };
  virtual void visit(CASTRef *peer) { assert(false); };
  virtual void visit(CASTIndirectTypeSpecifier *peer) { assert(false); };
  virtual void visit(CASTStatementExpression *peer) { assert(false); };
  virtual void visit(CASTContinueStatement *peer) { assert(false); };
};

class CASTDFSVisitor : public CASTVisitor

{
public:
  virtual void visit(CASTBase *peer) { assert(false); };
  virtual void visit(CASTFile *peer);
  virtual void visit(CASTPointer *peer) {};
  virtual void visit(CASTExpression *peer) {};
  virtual void visit(CASTAttributeSpecifier *peer);
  virtual void visit(CASTDeclarator *peer);
  virtual void visit(CASTIdentifier *peer) {};
  virtual void visit(CASTEllipsis *peer) {};
  virtual void visit(CASTNestedIdentifier *peer);
  virtual void visit(CASTUnaryOp *peer);
  virtual void visit(CASTCastOp *peer);
  virtual void visit(CASTPostfixOp *peer);
  virtual void visit(CASTNewOp *peer);
  virtual void visit(CASTIndexOp *peer);
  virtual void visit(CASTBrackets *peer);
  virtual void visit(CASTBinaryOp *peer);
  virtual void visit(CASTConditionalExpression *peer);
  virtual void visit(CASTArrayInitializer *peer);
  virtual void visit(CASTFunctionOp *peer);
  virtual void visit(CASTConstant *peer) {};
  virtual void visit(CASTIntegerConstant *peer) {};
  virtual void visit(CASTHexConstant *peer) {};
  virtual void visit(CASTFloatConstant *peer) {};
  virtual void visit(CASTStringConstant *peer) {};
  virtual void visit(CASTCharConstant *peer) {};
  virtual void visit(CASTDefaultConstant *peer) {};
  virtual void visit(CASTStorageClassSpecifier *peer) {};
  virtual void visit(CASTConstOrVolatileSpecifier *peer) {};
  virtual void visit(CASTTypeSpecifier *peer);
  virtual void visit(CASTDeclaration *peer);
  virtual void visit(CASTEmptyDeclaration *peer) {};
  virtual void visit(CASTStatement *peer) {};
  virtual void visit(CASTPreprocDefine *peer);
  virtual void visit(CASTPreprocConditional *peer);
  virtual void visit(CASTPreprocInclude *peer) {};
  virtual void visit(CASTPreprocUndef *peer) {};
  virtual void visit(CASTPreprocError *peer) {};
  virtual void visit(CASTComment *peer) {};
  virtual void visit(CASTBlockComment *peer) {};
  virtual void visit(CASTSpacer *peer) {};
  virtual void visit(CASTDeclarationStatement *peer);
  virtual void visit(CASTBaseClass *peer) {};
  virtual void visit(CASTAggregateSpecifier *peer);
  virtual void visit(CASTEnumSpecifier *peer);
  virtual void visit(CASTCompoundStatement *peer);
  virtual void visit(CASTExpressionStatement *peer);
  virtual void visit(CASTJumpStatement *peer) {};
  virtual void visit(CASTReturnStatement *peer);
  virtual void visit(CASTBreakStatement *peer) {};
  virtual void visit(CASTIterationStatement *peer) {};
  virtual void visit(CASTDoWhileStatement *peer);
  virtual void visit(CASTWhileStatement *peer);
  virtual void visit(CASTForStatement *peer);
  virtual void visit(CASTSelectionStatement *peer) {};
  virtual void visit(CASTIfStatement *peer);
  virtual void visit(CASTAsmInstruction *peer) {};
  virtual void visit(CASTAsmConstraint *peer);
  virtual void visit(CASTAsmStatement *peer);
  virtual void visit(CASTDeclarationExpression *peer);
  virtual void visit(CASTExternStatement *peer);
  virtual void visit(CASTEmptyStatement *peer) {};
  virtual void visit(CASTBooleanConstant *peer) {};
  virtual void visit(CASTBuiltinConstant *peer) {};
  virtual void visit(CASTConstructorDeclaration *peer);
  virtual void visit(CASTGotoStatement *peer);
  virtual void visit(CASTCaseStatement *peer);
  virtual void visit(CASTSwitchStatement *peer);
  virtual void visit(CASTLabelStatement *peer);
  virtual void visit(CASTTemplateStatement *peer);
  virtual void visit(CASTTemplateTypeDeclaration *peer);
  virtual void visit(CASTTemplateTypeIdentifier *peer);
  virtual void visit(CASTOperatorIdentifier *peer) {};
  virtual void visit(CASTRef *peer) {};
  virtual void visit(CASTIndirectTypeSpecifier *peer);
  virtual void visit(CASTStatementExpression *peer);
  virtual void visit(CASTContinueStatement *peer) {};
};

#endif
