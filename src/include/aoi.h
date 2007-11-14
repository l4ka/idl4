#ifndef __aoi_h__
#define __aoi_h__

#include <assert.h>

#include "base.h"
#include "globals.h"
#include "fe/compiler.h"

#define UNKNOWN 		(-1)

#define FLAG_IN 	(1<<0)
#define FLAG_OUT 	(1<<1)
#define FLAG_ONEWAY 	(1<<2)
#define FLAG_LOCAL 	(1<<3)
#define FLAG_REF	(1<<4)
#define FLAG_MAP	(1<<5)
#define FLAG_GRANT	(1<<6)
#define FLAG_WRITABLE	(1<<7)
#define FLAG_NOCACHE	(1<<8)
#define FLAG_L1_ONLY	(1<<9)
#define FLAG_ALL_CACHES	(1<<10)
#define FLAG_FOLLOW	(1<<11)
#define FLAG_NOXFER	(1<<12)
#define FLAG_READONLY	(1<<13)
#define FLAG_DONTDEFINE (1<<14)
#define FLAG_DONTSCOPE  (1<<15)
#define FLAG_PREALLOC	(1<<16)
#define FLAG_KERNELMSG	(1<<17)

#define forAll(a, b) \
  do { \
       for (CAoiBase *item = (a)->getFirstElement(); !item->isEndOfList(); item = item->next) \
         do { b; } while (0); \
     } while (0)

#define semanticError(ctxt, info...) \
  do { (ctxt)->showMessage(mprintf(info)); semanticErrors++; } while (0)

#define compilerError(ctxt, info...) \
  do { (ctxt)->showMessage(mprintf(info)); semanticErrors++; } while (0)

extern int semanticErrors;

class CAoiVisitor;

class CAoiContext : public CContext

{
public:
  CAoiContext(const char *file, const char *line, const int lineNo, const int pos) : CContext(file, line, lineNo, pos)
    { };
};

class CAoiBase : public CBase

{
public:
  const char *name;
  void *peer;
  CAoiBase *prev, *next, *parent;
  CAoiContext *context;

  CAoiBase(CAoiContext *context);
  void add(CAoiBase *newElement);
  virtual void accept(CAoiVisitor *worker);

  virtual bool isEndOfList() { return false; };
  virtual bool isScope() { return false; };
  virtual bool isType() { return false; };
  virtual bool isConstant() { return false; };
  virtual bool isReference() { return false; };
  virtual bool isException() { return false; };
  virtual bool isInterface() { return false; };
  virtual bool isStruct() { return false; };
  virtual bool isUnion() { return false; };
  virtual bool isAttribute() { return false; };
  virtual bool isOperation() { return false; };
  virtual bool isConstBase() { return false; };
  virtual bool isProperty() { return false; };
  virtual bool isParameter() { return false; };
};

class CAoiList : public CAoiBase

{
public:
  CAoiList() : CAoiBase(NULL) { this->prev = this; this->next = this; };
  virtual void accept(CAoiVisitor *worker);

  virtual bool isEndOfList() { return true; };
  CAoiBase *getFirstElement() { assert(next!=NULL); return next; };
  CAoiBase *removeFirstElement();
  CAoiBase *getByName(const char *identifier);
  CAoiBase *removeElement(CAoiBase *element);
  void merge(CAoiList *list);
  bool isEmpty() { return ((this->next) == this); };
};

enum AOICONSTOP { OP_OR, OP_XOR, OP_AND, OP_LSHIFT, OP_RSHIFT, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_NEG, OP_INVERT };

class CAoiConstBase : public CAoiBase

{
public:
  CAoiConstBase(CAoiContext *context) : CAoiBase(context) { };
  virtual void accept(CAoiVisitor *worker) { assert(false); };

  virtual bool performOperation(AOICONSTOP oper, CAoiConstBase *argument) { assert(false); return false; };
  virtual bool isConstBase() { return true; };
  virtual bool isScalar() { return false; };
  virtual bool isString() { return false; };
  virtual bool isFloat() { return false; };
};  

class CAoiConstInt : public CAoiConstBase

{
public:
  int value;
  
  CAoiConstInt(int value, CAoiContext *context) : CAoiConstBase(context) { this->value = value; };
  virtual void accept(CAoiVisitor *worker);

  virtual bool isScalar() { return true; };
  virtual bool performOperation(AOICONSTOP oper, CAoiConstBase *argument);
};

class CAoiConstString : public CAoiConstBase

{
public:
  const char *value;
  bool isWide;
  
  CAoiConstString(const char *value, CAoiContext *context) : CAoiConstBase(context) 
    { this->value = value; this->isWide = false; };
  virtual void accept(CAoiVisitor *worker);

  virtual bool isString() { return true; };
  virtual bool performOperation(AOICONSTOP oper, CAoiConstBase *argument);
};

class CAoiConstChar : public CAoiConstInt

{
public:
  CAoiConstChar(char value, CAoiContext *context) : CAoiConstInt(value, context) {};
  virtual void accept(CAoiVisitor *worker);
  virtual bool performOperation(AOICONSTOP oper, CAoiConstBase *argument);
};

class CAoiConstFloat : public CAoiConstBase

{
public:
  long double value;
  
  CAoiConstFloat(long double value, CAoiContext *context) : CAoiConstBase(context) { this->value = value; };
  virtual void accept(CAoiVisitor *worker);

  virtual bool isFloat() { return true; };
  virtual bool performOperation(AOICONSTOP oper, CAoiConstBase *argument);
};

class CAoiConstDefault : public CAoiConstBase

{
public:
  CAoiConstDefault(CAoiContext *context) : CAoiConstBase(context) { };
  virtual void accept(CAoiVisitor *worker);

  virtual bool performOperation(AOICONSTOP oper, CAoiConstBase *argument) { return false; };
};

class CAoiDeclarator : public CAoiBase

{
public:
  int refLevel;
  int dimension[MAX_DIMENSION];
  int numBrackets;
  int bits;
  
  CAoiDeclarator(CAoiContext *context) : CAoiBase(context) 
    { this->refLevel = 0; this->numBrackets = 0; this->bits = 0; };
  CAoiDeclarator(CAoiDeclarator *peer, CAoiContext *context) : CAoiBase(context) 
    { this->refLevel = 0; this->numBrackets = 0; this->bits = 0; merge(peer); };
  CAoiDeclarator(const char *name, CAoiContext *context) : CAoiBase(context) 
    { this->refLevel=0; this->numBrackets=0; this->name = name; this->bits = 0; };
  virtual void accept(CAoiVisitor *worker);

  void addDimension(int number) { if (numBrackets<MAX_DIMENSION) dimension[numBrackets++] = number; };
  void merge(CAoiDeclarator *peer);
  void addIndirection() { refLevel++; };
};

enum AOISYMBOLCLASS { SYM_SCOPE, SYM_TYPE, SYM_ATTRIBUTE, SYM_CONSTANT, SYM_OPERATION, SYM_EXCEPTION, SYM_ANY };

class CAoiScope : public CAoiBase

{
public:
  const char *scopedName;
  
  CAoiList *submodules;
  CAoiList *interfaces;
  CAoiList *types;
  CAoiList *attributes;
  CAoiList *constants;
  CAoiList *operations;
  CAoiList *exceptions;
  CAoiList *includes;

  CAoiScope(CAoiScope *parentScope, CAoiContext *context);
  virtual void accept(CAoiVisitor *worker);

  CAoiBase *lookupSymbol(const char *identifier, AOISYMBOLCLASS symbolType);
  virtual CAoiBase *localLookup(const char *identifier, AOISYMBOLCLASS symbolType);
  virtual bool isScope() { return true; };
};

class CAoiModule : public CAoiScope

{
public:
  int flags;

  CAoiModule(CAoiScope *parentScope, const char *identifier, CAoiContext *context) : CAoiScope(parentScope, context) 
    { this->name = identifier; this->flags = 0; };
  virtual void accept(CAoiVisitor *worker);
};

class CAoiProperty : public CAoiBase

{
public:
  CAoiConstBase *constValue;
  const char *refValue;

  CAoiProperty(const char *identifier, CAoiContext *context) : CAoiBase(context) 
    { this->name = identifier; this->constValue = NULL; this->refValue = NULL; };
  CAoiProperty(const char *identifier, CAoiConstBase *constValue, CAoiContext *context) : CAoiBase(context) 
    { this->name = identifier; this->constValue = constValue; this->refValue = NULL; };
  CAoiProperty(const char *identifier, const char *refValue, CAoiContext *context) : CAoiBase(context) 
    { this->name = identifier; this->refValue = refValue; this->constValue = NULL; };
  virtual void accept(CAoiVisitor *worker);

  virtual bool isProperty() { return true; };
};

class CAoiInterface : public CAoiScope

{
public:
  CAoiList *properties;
  CAoiList *superclasses;
  int flags;
  int iid;
  
  CAoiInterface(CAoiScope *parentScope, const char *identifier, CAoiList *superclasses, CAoiList *properties, CAoiContext *context) : CAoiScope(parentScope, context) 
    { this->name = identifier; this->superclasses = superclasses; this->flags = 0; this->iid = 0; this->properties = properties; mergeProperties(); };
  virtual void accept(CAoiVisitor *worker);
  void mergeProperties();
  CAoiList *getAllSuperclasses();

  virtual bool isInterface() { return true; };
};

class CAoiRootScope : public CAoiScope

{
public:
  CAoiRootScope();
  virtual void accept(CAoiVisitor *worker);
};

class CAoiType : public CAoiBase

{
public:
  CAoiScope *parentScope;
  int flags;

  CAoiType(const char *name, CAoiScope *parentScope, CAoiContext *context) : CAoiBase(context) 
    { this->name = name; this->parentScope = parentScope; this->flags = 0; };
  virtual void accept(CAoiVisitor *worker);

  virtual bool canAssign(CAoiConstBase *cnst) { return false; };
  virtual bool isType() { return true; };
  virtual bool isVoid() { return false; };
};

class CAoiStructType : public CAoiType

{
public:
  CAoiList *members;

  CAoiStructType(CAoiList *members, const char *identifier, CAoiScope *parentScope, CAoiContext *context) : CAoiType(identifier, parentScope, context) 
    { this->members = members; };
  virtual void accept(CAoiVisitor *worker);

  virtual bool isStruct() { return true; };
};

class CAoiUnionType : public CAoiType

{
public:
  CAoiType *switchType;
  CAoiList *members;
  
  CAoiUnionType(CAoiList *members, const char *identifier, CAoiType *switchType, CAoiScope *parentScope, CAoiContext *context) : CAoiType(identifier, parentScope, context) 
    { this->members = members; this->switchType = switchType; };
  virtual void accept(CAoiVisitor *worker);

  virtual bool isUnion() { return true; };
};

class CAoiEnumType : public CAoiType

{
public:
  CAoiList *members;

  CAoiEnumType(CAoiList *members, const char *identifier, CAoiScope *parentScope, CAoiContext *context) : CAoiType(identifier, parentScope, context) 
    { this->members = members; };
  virtual void accept(CAoiVisitor *worker);
};

class CAoiExceptionType : public CAoiStructType

{
public:
  CAoiExceptionType(CAoiList *members, const char *identifier, CAoiScope *parentScope, CAoiContext *context) : CAoiStructType(members, identifier, parentScope, context) 
    { };
  virtual void accept(CAoiVisitor *worker);

  virtual bool isException() { return true; };
};

class CAoiAliasType : public CAoiType

{
public:
  int dimension[MAX_DIMENSION];
  int numBrackets;
  CAoiType *ref;

  CAoiAliasType(CAoiType *ref, CAoiDeclarator *decl, CAoiScope *parentScope, CAoiContext *context);
  virtual void accept(CAoiVisitor *worker);

  virtual bool canAssign(CAoiConstBase *cnst);
};

class CAoiPointerType : public CAoiType

{
public:
  CAoiType *ref;

  CAoiPointerType(const char *name, CAoiType *ref, CAoiScope *parentScope, CAoiContext *context) : CAoiType(name, parentScope, context)
    { this->ref = ref; };
  virtual void accept(CAoiVisitor *worker);
};

class CAoiObjectType : public CAoiType

{
public:
  CAoiInterface *ref;

  CAoiObjectType(CAoiInterface *ref, CAoiScope *parentScope, CAoiContext *context) : CAoiType(ref ? ref->name : NULL, parentScope, context)
    { this->ref = ref; };
  virtual void accept(CAoiVisitor *worker);

  virtual bool canAssign(CAoiConstBase *cnst) { return false; };
};

class CAoiIntegerType : public CAoiType

{
public:
  const char *alias;
  int sizeInBytes;
  bool isSigned;
  
  CAoiIntegerType(const char *identifier, int sizeInBytes, bool isSigned, const char *alias, CAoiScope *parentScope, CAoiContext *context) : CAoiType(identifier, parentScope, context) 
    { this->sizeInBytes = sizeInBytes; this->isSigned = isSigned; this->alias = alias; };
  virtual void accept(CAoiVisitor *worker);
  virtual bool isVoid() { return (!sizeInBytes); };
  
  virtual bool canAssign(CAoiConstBase *cnst) 
    { return cnst->isScalar(); };
};

class CAoiWordType : public CAoiIntegerType

{
public:
  CAoiWordType(const char *identifier, bool isSigned, CAoiScope *parentScope, CAoiContext *context) : CAoiIntegerType(identifier, globals.word_size, isSigned, NULL, parentScope, context) 
    { };
  virtual void accept(CAoiVisitor *worker);
};

CAoiWordType *createAoiMachineWordType(CAoiScope *parentScope, CAoiContext *context);
CAoiWordType *createAoiSignedMachineWordType(CAoiScope *parentScope, CAoiContext *context);

class CAoiFloatType : public CAoiType

{
public:
  int sizeInBytes;
  const char *alias;
  
  CAoiFloatType(const char *identifier, int sizeInBytes, const char *alias, CAoiScope *parentScope, CAoiContext *context) : CAoiType(identifier, parentScope, context) 
    { this->sizeInBytes = sizeInBytes; this->alias = alias; };
  virtual void accept(CAoiVisitor *worker);
  
  virtual bool canAssign(CAoiConstBase *cnst) 
    { return cnst->isFloat(); };
};

class CAoiStringType : public CAoiType

{
public:
  CAoiType *elementType;
  int maxLength;
  
  CAoiStringType(int maxLength, CAoiType *elementType, CAoiScope *parentScope, CAoiContext *context) : CAoiType(NULL, parentScope, context) 
    { this->maxLength = maxLength; this->elementType = elementType; };
  virtual void accept(CAoiVisitor *worker);
  virtual bool canAssign(CAoiConstBase *cnst) 
    { return cnst->isString(); };
};

class CAoiFixedType : public CAoiType

{
public:
  int totalDigits, postComma;

  CAoiFixedType(const char *identifier, int totalDigits, int postComma, CAoiScope *parentScope, CAoiContext *context) : CAoiType(identifier, parentScope, context) 
    { this->totalDigits = totalDigits; this->postComma = postComma; };
  virtual void accept(CAoiVisitor *worker);
};  

class CAoiSequenceType : public CAoiType

{
public:
  CAoiType *elementType;
  int maxLength;

  CAoiSequenceType(CAoiType *elementType, int maxLength, CAoiScope *parentScope, CAoiContext *context) : CAoiType(NULL, parentScope, context) 
    { this->elementType = elementType; this->maxLength = maxLength; };
  virtual void accept(CAoiVisitor *worker);
};  

class CAoiCustomType : public CAoiType

{
public:
  const char *identifier;
  int size;
  
  CAoiCustomType(const char *identifier, int size, CAoiScope *parentScope, CAoiContext *context) : CAoiType(identifier, parentScope, context) 
    { this->size = size; this->identifier = identifier; };
  virtual void accept(CAoiVisitor *worker);
};

CAoiCustomType *createAoiThreadIDType(CAoiScope *parentScope, CAoiContext *context);

class CAoiRef : public CAoiBase

{
public:
  CAoiBase *ref;
  
  CAoiRef(CAoiBase *ref, CAoiContext *context) : CAoiBase(context) 
    { this->ref = ref; };
  virtual void accept(CAoiVisitor *worker);

  virtual bool isReference() { return true; };
};  

class CAoiConstant : public CAoiBase

{
public:
  CAoiScope *parentScope;
  CAoiType *type;
  CAoiConstBase *value;
  int flags;
  
  CAoiConstant(const char *name, CAoiType *type, CAoiConstBase *value, CAoiScope *parentScope, CAoiContext *context);
  virtual void accept(CAoiVisitor *worker);

  virtual bool isConstant() { return true; };
};

class CAoiParameter : public CAoiDeclarator

{
public:
  int flags;
  CAoiType *type;
  CAoiList *properties;
  
  CAoiParameter(CAoiParameter *peer, CAoiContext *context) : CAoiDeclarator(peer, context) 
    { this->properties = new CAoiList(); this->flags = 0; this->type = NULL; merge(peer); mergeProperties(); };
  CAoiParameter(CAoiType *type, CAoiDeclarator *decl, CAoiList *properties, CAoiContext *context);
  virtual void accept(CAoiVisitor *worker);
  void mergeProperties();

  virtual bool isParameter() { return true; };
  CAoiScope *getParentScope() { CAoiBase *tmp = parent; while ((tmp) && (!tmp->isScope())) tmp = tmp->parent; assert(tmp); return (CAoiScope*)tmp; };
  void merge(CAoiParameter *peer) { this->type = peer->type; this->flags |= peer->flags; this->parent = peer->parent; };
  CAoiProperty *getProperty(const char *propName);
};

class CAoiUnionElement : public CAoiParameter

{
public:
  CAoiConstBase *discrim;

  CAoiUnionElement(CAoiConstBase *discrim, CAoiParameter *member, CAoiContext *context) : CAoiParameter(member, context) 
    { this->discrim = discrim; };
  virtual void accept(CAoiVisitor *worker);
};

class CAoiOperation : public CAoiBase

{
public:
  CAoiType *returnType;
  CAoiList *parameters;
  CAoiList *exceptions;
  CAoiList *properties;
  int flags;
  int fid;

  CAoiOperation(CAoiScope *parentScope, CAoiType *returnType, const char *name, CAoiList *parameters, CAoiList *exceptions, CAoiList *properties, CAoiContext *context);
  void mergeProperties();
  virtual void accept(CAoiVisitor *worker);
  virtual bool isOperation() { return true; };
};

class CAoiAttribute : public CAoiParameter

{
public:
  CAoiAttribute(CAoiType *type, CAoiDeclarator *decl, CAoiList *properties, CAoiContext *context) : CAoiParameter(type, decl, properties, context) 
    { };
  virtual void accept(CAoiVisitor *worker);
  virtual bool isAttribute() { return true; };
};

class CAoiFactory

{
public:
  virtual CAoiRootScope *getRootScope()
    { return new CAoiRootScope(); };
  virtual CAoiModule *buildModule(CAoiScope *parentScope, const char *identifier, CAoiContext *context) 
    { return new CAoiModule(parentScope, identifier, context); };
  virtual CAoiInterface *buildInterface(CAoiScope *parentScope, const char *identifier, CAoiList *superclasses, CAoiList *properties, CAoiContext *context)
    { return new CAoiInterface(parentScope, identifier, superclasses, properties, context); };
  virtual CAoiList *buildList()
    { return new CAoiList(); };    
  virtual CAoiRef *buildRef(CAoiBase *ref, CAoiContext *context)
    { return new CAoiRef(ref, context); };
  virtual CAoiConstant *buildConstant(const char *name, CAoiType *type, CAoiConstBase *value, CAoiScope *parentScope, CAoiContext *context)
    { return new CAoiConstant(name, type, value, parentScope, context); };
  virtual CAoiConstInt *buildConstInt(int value, CAoiContext *context)
    { return new CAoiConstInt(value, context); };
  virtual CAoiConstString *buildConstString(const char *value, CAoiContext *context)
    { return new CAoiConstString(value, context); };
  virtual CAoiConstChar *buildConstChar(char value, CAoiContext *context)
    { return new CAoiConstChar(value, context); };
  virtual CAoiConstFloat *buildConstFloat(long double value, CAoiContext *context)
    { return new CAoiConstFloat(value, context); };
  virtual CAoiAliasType *buildAliasType(CAoiType *ref, CAoiDeclarator *decl, CAoiScope *parentScope, CAoiContext *context)
    { return new CAoiAliasType(ref, decl, parentScope, context); };
  virtual CAoiPointerType *buildPointerType(const char *name, CAoiType *ref, CAoiScope *parentScope, CAoiContext *context)
    { return new CAoiPointerType(name, ref, parentScope, context); };
  virtual CAoiObjectType *buildObjectType(CAoiInterface *ref, CAoiScope *parentScope, CAoiContext *context)
    { return new CAoiObjectType(ref, parentScope, context); };
  virtual CAoiDeclarator *buildDeclarator(CAoiContext *context)
    { return new CAoiDeclarator(context); };
  virtual CAoiDeclarator *buildDeclarator(CAoiDeclarator *peer, CAoiContext *context)
    { return new CAoiDeclarator(peer, context); };
  virtual CAoiDeclarator *buildDeclarator(const char *name, CAoiContext *context)
    { return new CAoiDeclarator(name, context); };
  virtual CAoiStructType *buildStructType(CAoiList *members, const char *identifier, CAoiScope *parentScope, CAoiContext *context)
    { return new CAoiStructType(members, identifier, parentScope, context); };
  virtual CAoiParameter *buildParameter(CAoiParameter *peer, CAoiContext *context)
    { return new CAoiParameter(peer, context); };
  virtual CAoiParameter *buildParameter(CAoiType *type, CAoiDeclarator *decl, CAoiList *properties, CAoiContext *context)
    { return new CAoiParameter(type, decl, properties, context); };
  virtual CAoiUnionType *buildUnionType(CAoiList *members, const char *identifier, CAoiType *switchType, CAoiScope *parentScope, CAoiContext *context)
    { return new CAoiUnionType(members, identifier, switchType, parentScope, context); };
  virtual CAoiUnionElement *buildUnionElement(CAoiConstBase *discrim, CAoiParameter *member, CAoiContext *context)
    { return new CAoiUnionElement(discrim, member, context); };
  virtual CAoiConstDefault *buildConstDefault(CAoiContext *context)
    { return new CAoiConstDefault(context); };
  virtual CAoiEnumType *buildEnumType(CAoiList *members, const char *identifier, CAoiScope *parentScope, CAoiContext *context)
    { return new CAoiEnumType(members, identifier, parentScope, context); };
  virtual CAoiExceptionType *buildExceptionType(CAoiList *members, const char *identifier, CAoiScope *parentScope, CAoiContext *context)
    { return new CAoiExceptionType(members, identifier, parentScope, context); };
  virtual CAoiStringType *buildStringType(int maxLength, CAoiType *elementType, CAoiScope *parentScope, CAoiContext *context)
    { return new CAoiStringType(maxLength, elementType, parentScope, context); };
  virtual CAoiOperation *buildOperation(CAoiScope *parentScope, CAoiType *returnType, const char *name, CAoiList *parameters, CAoiList *exceptions, CAoiList *properties, CAoiContext *context)
    { return new CAoiOperation(parentScope, returnType,  name, parameters, exceptions, properties, context); };
  virtual CAoiProperty *buildProperty(const char *identifier, CAoiContext *context)
    { return new CAoiProperty(identifier, context); };
  virtual CAoiProperty *buildProperty(const char *identifier, CAoiConstBase *constValue, CAoiContext *context)
    { return new CAoiProperty(identifier, constValue, context); };
  virtual CAoiProperty *buildProperty(const char *identifier, const char *refValue, CAoiContext *context)
    { return new CAoiProperty(identifier, refValue, context); };
  virtual CAoiAttribute *buildAttribute(CAoiType *type, CAoiDeclarator *decl, CAoiList *properties, CAoiContext *context)
    { return new CAoiAttribute(type, decl, properties, context); };
  virtual CAoiFixedType *buildFixedType(const char *identifier, int totalDigits, int postComma, CAoiScope *parentScope, CAoiContext *context)
    { return new CAoiFixedType(identifier, totalDigits, postComma, parentScope, context); };
  virtual CAoiSequenceType *buildSequenceType(CAoiType *elementType, int maxLength, CAoiScope *parentScope, CAoiContext *context)
    { return new CAoiSequenceType(elementType, maxLength, parentScope, context); };
    
  virtual const char *getDescription() { return "CORBA C"; };
  CAoiFactory() {};
};

class CAoiVisitor : public CBase

{
public:
  virtual void visit(CAoiBase *peer) { assert(false); };
  virtual void visit(CAoiList *peer) { assert(false); };
  virtual void visit(CAoiConstInt *peer) { assert(false); };
  virtual void visit(CAoiConstString *peer) { assert(false); };
  virtual void visit(CAoiConstChar *peer) { assert(false); };
  virtual void visit(CAoiConstFloat *peer) { assert(false); };
  virtual void visit(CAoiConstDefault *peer) { assert(false); };
  virtual void visit(CAoiDeclarator *peer) { assert(false); };
  virtual void visit(CAoiScope *peer) { assert(false); };
  virtual void visit(CAoiModule *peer) { assert(false); };
  virtual void visit(CAoiInterface *peer) { assert(false); };
  virtual void visit(CAoiRootScope *peer) { assert(false); };
  virtual void visit(CAoiType *peer) { assert(false); };
  virtual void visit(CAoiFloatType *peer) { assert(false); };
  virtual void visit(CAoiIntegerType *peer) { assert(false); };
  virtual void visit(CAoiWordType *peer) { assert(false); };
  virtual void visit(CAoiFixedType *peer) { assert(false); };
  virtual void visit(CAoiSequenceType *peer) { assert(false); };
  virtual void visit(CAoiCustomType *peer) { assert(false); };
  virtual void visit(CAoiStringType *peer) { assert(false); };
  virtual void visit(CAoiStructType *peer) { assert(false); };
  virtual void visit(CAoiUnionType *peer) { assert(false); };
  virtual void visit(CAoiEnumType *peer) { assert(false); };
  virtual void visit(CAoiExceptionType *peer) { assert(false); };
  virtual void visit(CAoiAliasType *peer) { assert(false); };
  virtual void visit(CAoiPointerType *peer) { assert(false); };
  virtual void visit(CAoiObjectType *peer) { assert(false); };
  virtual void visit(CAoiRef *peer) { assert(false); };
  virtual void visit(CAoiConstant *peer) { assert(false); };
  virtual void visit(CAoiParameter *peer) { assert(false); };
  virtual void visit(CAoiUnionElement *peer) { assert(false); };
  virtual void visit(CAoiProperty *peer) { assert(false); };
  virtual void visit(CAoiOperation *peer) { assert(false); };
  virtual void visit(CAoiAttribute *peer) { assert(false); };
};

#endif
