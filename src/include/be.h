#ifndef __be_h__
#define __be_h__

#include <string.h>
#include <assert.h>

#include "aoi.h"
#include "base.h"
#include "cast.h"

#define OP_PROVIDEBUFFER	(1<<0)
#define IID_KERNEL		(-42)

#define addTo(field, stuff) do { if (field) field->add(stuff); else field = (stuff); } while (0)
#define addWithTrailingSpacerTo(a, b) do { addTo((a), (b)); if (b) addTo((a), new CASTSpacer()); } while (0)
#define addWithLeadingSpacerTo(a, b) do { if ((b) && (a)) addTo((a), new CASTSpacer()); addTo((a), (b)); } while (0)

#define getInterface(aoi) ((CBEInterface*)((aoi)->peer))
#define getModule(aoi) ((CBEModule*)((aoi)->peer))
#define getType(aoi) ((CBEType*)((aoi)->peer))
#define getConstant(aoi) ((CBEConstant*)((aoi)->peer))
#define getConstBase(aoi) ((CBEConstBase*)((aoi)->peer))
#define getParameter(aoi) ((CBEParameter*)((aoi)->peer))
#define getScope(aoi) ((CBEScope*)((aoi)->peer))
#define getOperation(aoi) ((CBEOperation*)((aoi)->peer))
#define getException(aoi) ((CBEExceptionType*)((aoi)->peer))
#define getUnionElement(aoi) ((CBEUnionElement*)((aoi)->peer))
#define getRef(aoi) ((CBERef*)((aoi)->peer))

#define useMallocFor(param) (!(((param)->aoi->flags&FLAG_PREALLOC) || (!(globals.flags&FLAG_USEMALLOC))))

#define forAllBE(a, b) \
  do { \
       for (CBEBase *item = (a)->getFirstElement(); !item->isEndOfList(); item = item->next) \
         do { b; } while (0); \
     } while (0)

#define forAllBEReverse(a, b) \
  do { \
       for (CBEBase *item = (a)->getLastElement(); !item->isEndOfList(); item = item->prev) \
         do { b; } while (0); \
     } while (0)

/* Helpers */

CASTIdentifier *knitScopedNameFrom(const char *name);
CASTDeclarator *knitDeclarator(const char *name);
CASTDeclarator *knitIndirectDeclarator(const char *name);
CASTFunctionOp *knitFunctionOp(const char *name, CASTExpression *arguments);
CASTExpression *knitExprList(CASTExpression *a, CASTExpression *b = NULL, CASTExpression *c = NULL, CASTExpression *d = NULL, CASTExpression *e = NULL);
CASTDeclarator *knitDeclList(CASTDeclarator *a, CASTDeclarator *b = NULL, CASTDeclarator *c = NULL);
CASTStatement *knitStmtList(CASTStatement *a, CASTStatement *b = NULL, CASTStatement *c = NULL);
CASTExpression *knitPathInstance(CASTExpression *pathToItem, const char *instance);
CASTExpression *knitPeerInstance(CASTExpression *pathToItem, const char *instance, const char *peer);
CASTStatement *knitBlock(CASTStatement *statements);

class CBEMarshalOp;
class CBEParameter;
class CMSConnection;
class CMSService;

class CBEBase : public CBase

{
private:
  CAoiBase *aoi;
  
public:
  CBEBase *prev, *next;
  
  CBEBase(CAoiBase *aoi) : CBase() { this->aoi = aoi; this->prev = NULL; this->next = NULL; };
  virtual bool isEndOfList() { return false; };
  void add(CBEBase *newElement);
};

class CBEList : public CBEBase

{
public:
  CBEList() : CBEBase(NULL) { this->prev = this; this->next = this; };
  virtual bool isEndOfList() { return true; };
  CBEBase *getFirstElement() { assert(next!=NULL); return next; };
  CBEBase *getLastElement() { assert(prev!=NULL); return prev; };
  bool isEmpty() { return ((this->next) == this); };
};


class CBEIDSource;

class CBEScope : public CBEBase

{
public:
  CAoiScope *aoi;

  CBEScope(CAoiScope *aoi) : CBEBase(aoi) { this->aoi = aoi; };

  virtual CASTStatement *buildTestDeclarations();
  virtual void avoidConflicts(CBEScope *invoker, CBEIDSource *source);
  virtual void claimIDs(CBEIDSource *source);
  virtual void marshal();
  void assignIDs(int functionStart, int exceptionStart);
};

class CBEIDConstraint

{
public:
  CBEScope *masterScope, *slaveScope;
  CBEIDConstraint *next;
  
  CBEIDConstraint(CBEScope *masterScope, CBEScope *slaveScope, CBEIDConstraint *next)
    { this->masterScope = masterScope; this->slaveScope = slaveScope; this->next = next; };
};

class CBEIDRequest

{
public:
  CBEScope *scope;
  int functionCount;
  int exceptionCount;
  CBEIDRequest *next;
  
  CBEIDRequest(CBEScope *scope, int functionCount, int exceptionCount, CBEIDRequest *next)
    { this->scope = scope; this->functionCount = functionCount; this->exceptionCount = exceptionCount; this->next = next; };
};

class CBEIDSource : CBEBase

{
private:
  CBEIDConstraint *constraints;
  CBEIDRequest *requests;
  
public:
  CBEIDSource() : CBEBase(NULL)
    { constraints = NULL; requests = NULL; };
  void claim(CBEScope *scope, int functionCount, int exceptionCount);
  void mustNotConflict(CBEScope *thisScope, CBEScope *bossScope);
  void commit();
  int indexOf(CBEScope *scope);
  CBEIDRequest *requestAt(int index);
};

class CBEOperation;

enum CBEDeclType { RETVAL, IN, OUT, INOUT, ELEMENT };

class CBEVarSource;

class CBEType : public CBEBase

{
private:
  CAoiType *aoi;

public:
  CBEType(CAoiType *aoi) : CBEBase(aoi) { this->aoi = aoi; };
  
  virtual const char *getName() { panic("Type: No type name"); return NULL; };

  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL) { assert(false); return NULL; };
  virtual CASTDeclaration *buildMessageDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL) { return buildDeclaration(decl, compound); };
  virtual CASTStatement *buildDefinition() { panic("Type: Cannot produce definition: %s", aoi->name); return NULL; };
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags) { panic("Type: Cannot produce marshalOp: %s", aoi->name); return NULL; };
  virtual CASTStatement *buildAssignment(CASTExpression *dest, CASTExpression *src) { return new CASTExpressionStatement(new CASTBinaryOp("=", dest, src)); };
  virtual CASTExpression *buildBufferAllocation(CASTExpression *elements) { return new CASTFunctionOp(new CASTIdentifier(mprintf("CORBA_%s_missing_alloc", aoi->name)), elements); };
  virtual CASTExpression *buildDefaultValue() { panic("Type: Cannot build default value for %s", aoi->name); return NULL; };

  virtual CASTStatement *buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param) { panic("Type: Cannot produce test client init: %s", aoi->name); return NULL; };
  virtual CASTStatement *buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type) { panic("Type: Cannot produce test client check: %s", aoi->name); return NULL; };
  virtual CASTStatement *buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type) { panic("Type: Cannot produce test client post: %s", aoi->name); return NULL; };
  virtual CASTStatement *buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type) { panic("Type: Cannot produce test client cleanup: %s", aoi->name); return NULL; };
  virtual CASTStatement *buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable) { panic("Type: Cannot produce test server init: %s", aoi->name); return NULL; };
  virtual CASTStatement *buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type) { panic("Type: Cannot produce test server check: %s", aoi->name); return NULL; };
  virtual CASTStatement *buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type) { panic("Type: Cannot produce test server recheck: %s", aoi->name); return NULL; };
  virtual CASTStatement *buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value) { panic("Type: Cannot produce test display statement: %s", aoi->name); return NULL; };
  
  virtual bool needsSpecialHandling() { panic("Type: Cannot provide special handling information for: %s", aoi->name); return false; };
  virtual bool needsDirectCopy() { panic("Type: Cannot provide direct copy information for: %s", aoi->name); return false; };
  virtual int getArgIndirLevel(CBEDeclType type) { panic("Type: Cannot determine argument indirection level: %s", aoi->name); return 0; };
  virtual int getFlatSize() { panic("Type: Cannot determine flat size for: %s", aoi->name); return UNKNOWN; };
  virtual int getAlignment() { panic("Type: Cannot determine alignment for: %s", aoi->name); return UNKNOWN; };
  virtual bool involvesMapping() { panic("Type: Cannot tell whether mapping is involved: %s", aoi->name); return false; };
  virtual CBEParameter *getFirstMemberWithProperty(const char *property) { return NULL; };
  virtual bool isScalar() { return false; };
  virtual bool isImplicitPointer() { return false; };
  virtual bool isAllowedParamType() { return !globals.compat_mode; };

  CASTIdentifier *buildIdentifier();
  CASTExpression *buildTypeCast(int indirLevel, CASTExpression *expr);
  CASTStatement *buildConditionalDefinition(CASTStatement *simpleDef);
  bool equals(CBEType *peer) { return (!strcmp(peer->aoi->name, aoi->name)); };
  const char *getDirectionString(CBEDeclType type);
};

class CBEVarSource

{
private:
  CBEVarSource *next;
  CBEType *type;
  const char *name;
  bool isPersistent;
  int indirLevel, id;

public:
  CBEVarSource(CBEType *type, const char *name, bool isPersistent, int indirLevel, int id) 
    { this->next = NULL; this->type = type; this->name = name; this->isPersistent = isPersistent; this->id = id; this->indirLevel = indirLevel; };
  CBEVarSource()
    { this->next = NULL; this->type = NULL; this->name = NULL; this->isPersistent = true; this->id = 0; this->indirLevel = 0; };
  CASTIdentifier *requestVariable(bool needPersistence, CBEType *type, int indirLevel = 0);
  CASTStatement *declareAll();
};    

class CBEDependency

{
public:
  CBEParameter *to;
  CBEDependency *next;
  bool done;
  
  CBEDependency(CBEParameter *to, CBEDependency *next = NULL)
    { this->to = to; this->next = next; this->done = false; }
};

class CBEParamDep

{
public:
  CBEDependency *dependencies;
  CBEParameter *param;
  CBEParamDep *next;
  bool done;
  
  bool hasUnresolvedDependencies();
  
  CBEParamDep(CBEParameter *param, CBEParamDep *next = NULL)
    { this->param = param; this->next = next; this->dependencies = NULL; this->done = false; }
};

class CBEDependencyList : public CBEBase

{
private:
  CBEParamDep *paramDeps;

  void removeAllDependenciesTo(CBEParameter *param);
  
public:
  CBEDependencyList() : CBEBase(NULL)
    { paramDeps = NULL; };
  void registerParam(CBEParameter *param);
  void registerDependency(CBEParameter *param, CBEParameter *dependency);
  bool commit();
};

class CBEParameter : public CBEBase

{
private:
  CBEDeclType type;
  
public:
  CAoiParameter *aoi;
  bool marshalled;
  int priority;
  
  CBEParameter(CAoiParameter *aoi);
    
  virtual CASTDeclaration *buildDeclaration();
  CASTIdentifier *buildIdentifier();

  CASTStatement *buildTestClientInit(CASTExpression *globalPath, CBEVarSource *varSource);
  CASTStatement *buildTestClientCheck(CASTExpression *globalPath, CBEVarSource *varSource);
  CASTStatement *buildTestClientPost(CASTExpression *globalPath, CBEVarSource *varSource);
  CASTStatement *buildTestClientCleanup(CASTExpression *globalPath, CBEVarSource *varSource);
  CASTStatement *buildTestServerInit(CASTExpression *globalPath, CBEVarSource *varSource, bool bufferAvailable);
  CASTStatement *buildTestServerCheck(CASTExpression *globalPath, CBEVarSource *varSource);
  CASTStatement *buildTestServerRecheck(CASTExpression *globalPath, CBEVarSource *varSource);
  CASTStatement *buildTestDisplayStmt(CASTExpression *globalPath, CBEVarSource *varSource, CASTExpression *value);
  
  CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int flags);
  CASTExpression *buildRvalue(CASTExpression *expr);
  int getArgIndirLevel() { return getType(aoi->type)->getArgIndirLevel(type); };
  CBEParameter *getPropertyParameter(const char *propName);
  CASTExpression *buildTestClientArg(CASTExpression *globalPath);
  void registerDependencies(CBEDependencyList *dependencyList);
  void assignPriority(int priority) { this->priority = priority; };
  bool isSentFromServerToClient() { return (type==INOUT) || (type==OUT); };
  CBEParameter *getFirstMemberWithProperty(const char *property);
};

class CBERootScope : public CBEScope

{
private:
  CAoiRootScope *aoi;

public:
  CBERootScope(CAoiRootScope *aoi) : CBEScope(aoi) { this->aoi = aoi; };
  
  virtual CASTStatement *buildTestDeclarations();
  CASTIdentifier *buildFileCode();
  CASTStatement *buildTitleComment(bool addWarning);
  CASTStatement *buildClientHeader();
  CASTStatement *buildClientCode();
  CASTStatement *buildServerHeader();
  CASTStatement *buildServerCode();
  CASTStatement *buildServerTemplate();
  CASTStatement *buildTestInvocation();
  CASTStatement *buildConfigOptions();
  CASTStatement *buildCustomIncludes();
  CASTStatement *buildIncludeCheck();
};

class CBEModule : public CBEScope

{
private:
  CAoiModule *aoi;
  
public:
  CBEModule(CAoiModule *aoi) : CBEScope(aoi) { this->aoi = aoi; };
  CASTStatement *buildClientHeader();
  CASTStatement *buildServerHeader();
  CASTStatement *buildTitleComment();
  CASTStatement *buildServerTemplate();
  CASTStatement *buildTestInvocation();
};  

class CBEInterface : public CBEScope

{
private: 
  CAoiInterface *aoi;
  CMSService *service;
  CBEList *inheritedOps;

public:
  CBEInterface(CAoiInterface *aoi, CBEList *inheritedOps) : CBEScope(aoi) 
    { this->aoi = aoi; this->service = NULL; this->inheritedOps = inheritedOps; };
  virtual void claimIDs(CBEIDSource *source);
  virtual void avoidConflicts(CBEScope *invoker, CBEIDSource *source);
  virtual void marshal();
  
  CASTStatement *buildTitleComment();
  CASTStatement *buildReferenceDefinition();
  CASTStatement *buildServerDefinitions();
  CASTIdentifier *buildIdentifier();
  CASTStatement *buildClientHeader();
  CASTStatement *buildServerHeader();
  CASTStatement *buildServerTemplate();
  CASTStatement *buildTestInvocation();
  CASTIdentifier *buildServerFuncName();
  CASTStatement *buildServerLoop();
  CASTIdentifier *buildVtableName(int iid);
  CASTIdentifier *buildKtableName();
  CASTIdentifier *buildDefaultVtableName();
  CASTStatement *buildVtableDeclaration();
  
  void getIDRange(int *iidMin, int *iidMax, int *fidMin, int *fidMax, int *kidMin, int *kidMax);
  bool containsKernelMessages();
  bool containsUserMessages();
  int getVtableSize();
  int getItableSize();
  int getKtableSize();
  CAoiOperation *getFunctionForID(int fid, int iid, CAoiScope **definitionScope);
};  

class CBEAliasType : public CBEType

{
private:
  CAoiAliasType *aoi;

public:
  CBEAliasType(CAoiAliasType *aoi) : CBEType(aoi) { this->aoi = aoi; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTDeclaration *buildMessageDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTStatement *buildDefinition();
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags);
  virtual CASTExpression *buildBufferAllocation(CASTExpression *elements) { return getType(aoi->ref)->buildBufferAllocation(elements); };
  virtual CASTExpression *buildDefaultValue();

  virtual CASTStatement *buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param);
  virtual CASTStatement *buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable);
  virtual CASTStatement *buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value);
  
  virtual int getArgIndirLevel(CBEDeclType type);
  virtual int getAlignment();
  virtual bool needsSpecialHandling();
  virtual bool needsDirectCopy();
  virtual int getFlatSize();
  virtual bool involvesMapping();
  virtual CBEParameter *getFirstMemberWithProperty(const char *property);
  virtual CASTStatement *buildAssignment(CASTExpression *dest, CASTExpression *src);
  virtual bool isImplicitPointer();
  virtual bool isAllowedParamType();
  void buildIndexExpression(CASTExpression **pathToItem, CASTIdentifier *loopVar[]);
  CASTStatement *buildArrayScan(CASTStatement *preStatement, CASTStatement *innerStatement, CASTStatement *postStatement, CASTIdentifier *loopVar[]);
};

class CBEPointerType : public CBEType

{
private:
  CAoiPointerType *aoi;

public:
  CBEPointerType(CAoiPointerType *aoi) : CBEType(aoi) { this->aoi = aoi; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTStatement *buildDefinition();
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags);
  virtual CASTExpression *buildDefaultValue();

  virtual CASTStatement *buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param);
  virtual CASTStatement *buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable);
  virtual CASTStatement *buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value);
  
  virtual int getArgIndirLevel(CBEDeclType type);
  virtual int getAlignment() { return globals.word_size; };
  virtual bool needsSpecialHandling() { return false; };
  virtual bool needsDirectCopy() { return true; };
  virtual int getFlatSize() { return globals.word_size; };
  virtual bool involvesMapping() { return false; };
  virtual CASTStatement *buildAssignment(CASTExpression *dest, CASTExpression *src);
};

class CBEThreadIDType : public CBEType

{
private:
  CAoiType *aoi;

public:
  CBEThreadIDType(CAoiType *aoi) : CBEType(aoi) { this->aoi = aoi; };
  virtual const char *getName() { return "threadid"; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTDeclaration *buildMessageDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags);
  virtual CASTStatement *buildDefinition() { return NULL; };
  virtual CASTExpression *buildDefaultValue();

  virtual CASTStatement *buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param);
  virtual CASTStatement *buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type) { return NULL; };
  virtual CASTStatement *buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type) { return NULL; };
  virtual CASTStatement *buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable);
  virtual CASTStatement *buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value);
  
  virtual int getArgIndirLevel(CBEDeclType type);
  virtual int getAlignment();
  virtual bool needsSpecialHandling() { return globals.compat_mode; };
  virtual bool needsDirectCopy() { return true; };
  virtual int getFlatSize();
  virtual bool involvesMapping() { return false; };
  virtual bool isAllowedParamType() { return true; };
};

class CBEObjectType : public CBEThreadIDType

{
private:
  CAoiObjectType *aoi;

public:
  CBEObjectType(CAoiObjectType *aoi) : CBEThreadIDType(aoi) { this->aoi = aoi; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
};

class CBEIntegerType : public CBEType

{
private: 
  CAoiIntegerType *aoi;

public: 
  CBEIntegerType(CAoiIntegerType *aoi) : CBEType(aoi) { this->aoi = aoi; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTStatement *buildDefinition() { return NULL; };
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags);
  virtual CASTExpression *buildBufferAllocation(CASTExpression *elements);
  virtual CASTExpression *buildDefaultValue();

  virtual CASTStatement *buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param);
  virtual CASTStatement *buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable);
  virtual CASTStatement *buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value);

  virtual bool needsSpecialHandling() { return false; };
  virtual bool needsDirectCopy() { return true; };
  virtual int getArgIndirLevel(CBEDeclType type);
  virtual int getFlatSize() { return aoi->sizeInBytes; };
  virtual int getAlignment();
  virtual bool isScalar() { return true; };
  virtual bool involvesMapping() { return false; };
  
  CASTStatement *buildTestServerCheckGeneric(CASTExpression *globalPath, CASTExpression *localPath, const char *text);
};

class CBEWordType : public CBEIntegerType

{
private: 
  CAoiWordType *aoi;

public:
  CBEWordType(CAoiWordType *aoi) : CBEIntegerType(aoi) { this->aoi = aoi; };
  virtual const char *getName() { return aoi->isSigned ? "signed_word" : "word"; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTDeclaration *buildMessageDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags);

  virtual bool needsSpecialHandling() { return globals.compat_mode && aoi->isSigned; };
  virtual bool isAllowedParamType() { return true; };
};

class CBEFloatType : public CBEType

{
private: 
  CAoiFloatType *aoi;

public: 
  CBEFloatType(CAoiFloatType *aoi) : CBEType(aoi) { this->aoi = aoi; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTStatement *buildDefinition() { return NULL; };
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags);
  virtual CASTExpression *buildBufferAllocation(CASTExpression *elements);
  virtual CASTExpression *buildDefaultValue();

  virtual CASTStatement *buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param);
  virtual CASTStatement *buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable);
  virtual CASTStatement *buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value);

  virtual bool needsSpecialHandling() { return false; };
  virtual bool needsDirectCopy() { return true; };
  virtual int getArgIndirLevel(CBEDeclType type);
  virtual int getAlignment() { return (aoi->sizeInBytes>globals.word_size) ? globals.word_size : aoi->sizeInBytes; };
  virtual int getFlatSize() { return aoi->sizeInBytes; };
  virtual bool involvesMapping() { return false; };

  CASTStatement *buildTestServerCheckGeneric(CASTExpression *globalPath, CASTExpression *localPath, const char *text);
};

class CBEStringType : public CBEType

{
private: 
  CAoiStringType *aoi;

public: 
  CBEStringType(CAoiStringType *aoi) : CBEType(aoi) { this->aoi = aoi; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTStatement *buildDefinition() { return NULL; };
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags);
  virtual CASTExpression *buildDefaultValue();

  virtual CASTStatement *buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param);
  virtual CASTStatement *buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable);
  virtual CASTStatement *buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value);

  virtual bool needsSpecialHandling() { return true; };
  virtual bool needsDirectCopy() { return false; };
  virtual int getArgIndirLevel(CBEDeclType type);
  virtual int getFlatSize() { return globals.word_size; };
  virtual int getAlignment() { return globals.word_size; };
  virtual bool involvesMapping() { return false; };
  virtual bool isAllowedParamType() { return true; };
};

class CBEFixedType : public CBEType

{
private: 
  CAoiFixedType *aoi;

public: 
  CBEFixedType(CAoiFixedType *aoi) : CBEType(aoi) { this->aoi = aoi; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTStatement *buildDefinition();
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags);
  virtual int getArgIndirLevel(CBEDeclType type);
};

class CBESequenceType : public CBEType

{
private: 
  CAoiSequenceType *aoi;

public: 
  CBESequenceType(CAoiSequenceType *aoi) : CBEType(aoi) { this->aoi = aoi; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTStatement *buildDefinition();
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags);
  virtual CASTExpression *buildDefaultValue();

  virtual CASTStatement *buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param);
  virtual CASTStatement *buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable);
  virtual CASTStatement *buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value);

  virtual bool needsSpecialHandling();
  virtual bool needsDirectCopy();
  virtual int getArgIndirLevel(CBEDeclType type);
  virtual int getFlatSize();
  virtual int getAlignment();
  virtual bool involvesMapping() { return false; };
  virtual bool isAllowedParamType();
};

class CBEFpageType : public CBEType

{
private: 
  CAoiCustomType *aoi;

public: 
  CBEFpageType(CAoiCustomType *aoi) : CBEType(aoi) { this->aoi = aoi; };
  virtual const char *getName() { return "fpage"; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTStatement *buildDefinition() { return NULL; };
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags);
  virtual CASTExpression *buildDefaultValue();

  virtual CASTStatement *buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param);
  virtual CASTStatement *buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable);
  virtual CASTStatement *buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value);

  virtual bool needsSpecialHandling() { return true; };
  virtual bool needsDirectCopy() { return true; };
  virtual int getArgIndirLevel(CBEDeclType type);
  virtual int getFlatSize() { return globals.word_size; };
  virtual int getAlignment() { return globals.word_size; };
  virtual bool involvesMapping() { return true; };
  virtual bool isAllowedParamType() { return true; };
};

class CBEConstant : public CBEBase

{
private:
  CAoiConstant *aoi;
  
public:
  CBEConstant(CAoiConstant *aoi) : CBEBase(aoi) { this->aoi = aoi; };
  virtual CASTStatement *buildDefinition();
};

class CBEAttribute : public CBEParameter

{
private:
  CAoiAttribute *aoi;
  
public:
  CBEAttribute(CAoiAttribute *aoi) : CBEParameter(aoi) { this->aoi = aoi; };
};

class CBEOperation : public CBEBase

{
protected:
  CAoiOperation *aoi;
  CBEMarshalOp *marshalOps;
  
public:
  CMSConnection *connection;
  CBEMarshalOp *returnOp;
  CBEList *sortedParams;
  int id;

  CBEOperation(CAoiOperation *aoi) : CBEBase(aoi) 
    { this->aoi = aoi; this->id = UNKNOWN; this->returnOp = NULL; 
      this->connection = NULL; this->marshalOps = NULL; 
      this->sortedParams = new CBEList(); this->check(); };
  void check();
  void assignID(int id);
  virtual CASTIdentifier *buildClassIdentifier();
  CASTIdentifier *buildIdentifier();
  CASTIdentifier *buildOriginalClassIdentifier();
  virtual CASTStatement *buildClientHeader();
  virtual CASTStatement *buildServerHeader();
  virtual CASTStatement *buildServerTemplate();
  virtual CASTCompoundStatement *buildServerStub();
  virtual CASTCompoundStatement *buildServerReplyStub();
  CASTIdentifier *buildWrapperName(bool capitalize);
  CASTIdentifier *buildTestFuncName();
  CASTStatement *buildTestFunction();
  CASTStatement *buildTestDeclarations();
  CASTIdentifier *buildTestKeyPrefix();
  virtual int getID();
  virtual void marshal(CMSService *service);
  CASTStatement *buildTestInvocation();
  CASTStatement *buildTestClientCode(CBEVarSource *varSource);
};

class CBEInheritedOperation : public CBEOperation

{
  CAoiInterface *actualParent;

public:
  CBEInheritedOperation(CAoiOperation *aoi, CAoiInterface *actualParent);
  virtual CASTIdentifier *buildClassIdentifier();
  virtual CASTStatement *buildClientHeader();
  virtual CASTStatement *buildServerHeader();
  virtual int getID();
};  

class CBEUnionElement : public CBEParameter

{
public:
  CAoiUnionElement *aoi;

  CBEUnionElement(CAoiUnionElement *aoi) : CBEParameter(aoi) { this->aoi = aoi; };
};

class CBEUnionType : public CBEType

{
private:
  CAoiUnionType *aoi;

public:
  CBEUnionType(CAoiUnionType *aoi) : CBEType(aoi) { this->aoi = aoi; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTStatement *buildDefinition();
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags);
  virtual int getArgIndirLevel(CBEDeclType type);
  virtual CASTExpression *buildDefaultValue();

  virtual CASTStatement *buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param);
  virtual CASTStatement *buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable);
  virtual CASTStatement *buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value);

  virtual bool needsSpecialHandling();
  virtual bool needsDirectCopy();
  virtual int getFlatSize();
  virtual int getAlignment();
  virtual bool involvesMapping() { return false; };

  CASTDeclarationSpecifier *buildSpecifier();
  CBEUnionElement *getLargestMember();
};

class CBEConstBase : public CBEBase

{
private:
  CAoiConstBase *aoi;
  
public:
  CBEConstBase(CAoiConstBase *aoi) : CBEBase(aoi) { this->aoi = aoi; };
  virtual CASTExpression *buildExpression() { assert(false);return NULL; };
};

class CBEConstInt : public CBEConstBase

{
private:
  CAoiConstInt *aoi;
  
public:
  CBEConstInt(CAoiConstInt *aoi) : CBEConstBase(aoi) { this->aoi = aoi; };
  virtual CASTExpression *buildExpression();
};

class CBEConstString : public CBEConstBase

{
private:
  CAoiConstString *aoi;
  
public:
  CBEConstString(CAoiConstString *aoi) : CBEConstBase(aoi) { this->aoi = aoi; };
  virtual CASTExpression *buildExpression();
};

class CBEConstChar : public CBEConstInt

{
private:
  CAoiConstChar *aoi;
  
public:
  CBEConstChar(CAoiConstChar *aoi) : CBEConstInt(aoi) { this->aoi = aoi; };
  virtual CASTExpression *buildExpression();
};

class CBEConstFloat : public CBEConstBase

{
private:
  CAoiConstFloat *aoi;
  
public:
  CBEConstFloat(CAoiConstFloat *aoi) : CBEConstBase(aoi) { this->aoi = aoi; };
  virtual CASTExpression *buildExpression();
};

class CBEConstDefault : public CBEConstBase

{
private:
  CAoiConstDefault *aoi;
  
public:
  CBEConstDefault(CAoiConstDefault *aoi) : CBEConstBase(aoi) { this->aoi = aoi; };
  virtual CASTExpression *buildExpression();
};

class CBEStructType : public CBEType

{
private:
  CAoiStructType *aoi;

public:
  CBEStructType(CAoiStructType *aoi) : CBEType(aoi) { this->aoi = aoi; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTStatement *buildDefinition();
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags);
  virtual CASTExpression *buildDefaultValue();
  virtual CASTExpression *buildBufferAllocation(CASTExpression *elements);

  virtual CASTStatement *buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param);
  virtual CASTStatement *buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable);
  virtual CASTStatement *buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value);

  virtual bool needsSpecialHandling();
  virtual bool needsDirectCopy();
  virtual int getArgIndirLevel(CBEDeclType type);
  virtual int getFlatSize();
  virtual int getAlignment();
  virtual CBEParameter *getFirstMemberWithProperty(const char *property);
  virtual bool involvesMapping() { return false; };
  
  CASTDeclarationSpecifier *buildSpecifier();
  CASTIdentifier *buildGlobalReference(CASTIdentifier *key);
  CASTExpression *buildRvalueGlobal(CASTExpression *anchor, CASTIdentifier *key, CASTIdentifier *extension);
  CASTExpression *buildRvalueLocal(CASTExpression *anchor, CASTIdentifier *key, CASTIdentifier *extension);
  void prepareIteration(CAoiParameter *param, CASTExpression **globalPath, CASTExpression **localPath, CASTIdentifier *loopVar[], CBEVarSource *varSource);
  CASTStatement *iterate(CAoiParameter *param, CASTStatement *memberTest, CASTIdentifier *loopVar[]);
};

class CBEEnumType : public CBEType

{
private:
  CAoiEnumType *aoi;

public:
  CBEEnumType(CAoiEnumType *aoi) : CBEType(aoi) { this->aoi = aoi; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTStatement *buildDefinition();
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags);
  virtual CASTExpression *buildDefaultValue();

  virtual CASTStatement *buildTestClientInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param);
  virtual CASTStatement *buildTestClientCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientPost(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestClientCleanup(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerInit(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CBEParameter *param, bool bufferAvailable);
  virtual CASTStatement *buildTestServerCheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestServerRecheck(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type);
  virtual CASTStatement *buildTestDisplayStmt(CASTExpression *globalPath, CASTExpression *localPath, CBEVarSource *varSource, CBEDeclType type, CASTExpression *value);

  virtual bool needsSpecialHandling() { return false; };
  virtual bool needsDirectCopy() { return true; };
  virtual int getArgIndirLevel(CBEDeclType type);
  virtual int getFlatSize() { return globals.word_size; };
  virtual int getAlignment() { return globals.word_size; };
  virtual bool isScalar() { return true; };
  virtual bool involvesMapping() { return false; };
  
  CASTStatement *buildTestServerCheckGeneric(CASTExpression *globalPath, CASTExpression *localPath, const char *text);
};

class CBEExceptionType : public CBEType

{
private:
  CAoiExceptionType *aoi;
  
public:
  int id;
  CBEExceptionType(CAoiExceptionType *aoi) : CBEType(aoi) { this->aoi = aoi; this->id = UNKNOWN; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual CASTStatement *buildDefinition();
  void assignID(int id) { this->id = id; };
  virtual CBEMarshalOp *buildMarshalOps(CMSConnection *connection, int channels, CASTExpression *rvalue, const char *name, CBEType *originalType, CBEParameter *param, int flags);
  virtual int getArgIndirLevel(CBEDeclType type);
  CASTIdentifier *buildID();
};

class CBERef : public CBEBase

{
private:
  CAoiRef *aoi;
  
public:
  CBERef(CAoiRef *aoi) : CBEBase(aoi) { this->aoi = aoi; };
};

class CBEOpaqueType : public CBEType

{
  int sizeInBytes, alignment;
  bool scalar;
  const char *name;
  
public: 
  CBEOpaqueType(const char *name, int sizeInBytes, int alignment, bool scalar = false) : CBEType(new CAoiCustomType(name, sizeInBytes, NULL, new CAoiContext("builtin", "opaque", 1, 1))) 
    { this->name = name; this->sizeInBytes = sizeInBytes; this->alignment = alignment; this->scalar = scalar; };
  virtual CASTDeclaration *buildDeclaration(CASTDeclarator *decl, CASTCompoundStatement *compound = NULL);
  virtual int getArgIndirLevel(CBEDeclType type);
  virtual bool needsSpecialHandling() { return true; };
  virtual bool needsDirectCopy() { return true; };
  virtual int getFlatSize() { return sizeInBytes; };
  virtual int getAlignment() { return alignment; };
  virtual bool isScalar() { return scalar; };
  virtual bool involvesMapping() { return false; };
};

class CBEAddIndirVisitor : public CASTDFSVisitor

{
public:
  bool seenTypedef;
  CBEAddIndirVisitor() : CASTDFSVisitor() { seenTypedef = false; };
  virtual void visit(CASTStorageClassSpecifier *peer);
  virtual void visit(CASTDeclarator *peer);
};

#endif
