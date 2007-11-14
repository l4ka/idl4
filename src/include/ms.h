#ifndef __ms_h__
#define __ms_h__

#include <unistd.h>
#include <assert.h>

#include "base.h"
#include "globals.h"
#include "cast.h"
#include "aoi.h"
#include "be.h"

#define CHANNEL_IN 0
#define CHANNEL_OUT 1
#define CHANNEL_MASK(c)	(1<<(c))
#define CHANNEL_ALL_OUT (0xFFFFFFFF - CHANNEL_MASK(CHANNEL_IN))

#define OPTION_ONEWAY 	(1<<0)
#define OPTION_LOCAL 	(1<<1)
#define OPTION_FASTCALL	(1<<2)

#define HAS_IN_AND_OUT(c) 	(((c)&CHANNEL_MASK(CHANNEL_IN)) && ((c)&CHANNEL_MASK(CHANNEL_OUT)))
#define HAS_IN(c) 		((c)&CHANNEL_MASK(CHANNEL_IN))
#define HAS_OUT(c)		((c)&CHANNEL_MASK(CHANNEL_OUT))

#define forAllMS(a, b) \
  do { \
       for (CMSBase *item = (a)->getFirstElement(); !item->isEndOfList(); item = item->next) \
         do { b; } while (0); \
     } while (0)

typedef int ChunkID;

CASTExpression *buildSizeDwordAlign(CASTExpression *size, int elementSize, bool scaleToChars);
CAoiConstant *getBuiltinConstant(CAoiRootScope *rootScope, CAoiScope *parentScope, const char *name, int value);
CAoiModule *getBuiltinScope(CAoiScope *parentScope, const char *name);

class CMSBase : public CBase

{
public:
  int prio;
  const char *name;
  CMSBase *prev, *next, *parent;

  CMSBase() { this->name = NULL; this->prev = NULL; this->next = NULL; this->parent = NULL; this->prio = 0; };
  CMSBase(CAoiBase *aoiBase) { this->name = aoiBase->name; this->prev = NULL; this->next = NULL; this->parent = NULL; this->prio = 0; };
  void add(CMSBase *newElement);
  virtual bool isEndOfList() { return false; };
  virtual void dump() { assert(false); };
};

class CMSList : public CMSBase

{
public:
  CMSList() : CMSBase() { this->prev = this; this->next = this; };
  virtual bool isEndOfList() { return true; };
  CMSBase *getFirstElement() { assert(next!=NULL); return next; };
  CMSBase *removeFirstElement();
  CMSBase *getFirstElementWithPrio(int prio);
  CMSBase *getByName(const char *identifier);
  CMSBase *removeElement(CMSBase *element);
  bool hasPrioOrAbove(int prio);
  void merge(CMSList *list);
  void addWithPrio(CMSBase *newElement, int prio);
  bool isEmpty() { return ((this->next) == this); };
  virtual void dump(const char *title);
};

class CMSConnection;
class CMSService;
class CBEType;

class CMSChunk : public CMSBase

{
public:
  ChunkID chunkID;
  CBEType *type;
  int size;

  CMSChunk(ChunkID chunkID, int size, const char *name, CBEType *type) : CMSBase() { this->size = size; this->chunkID = chunkID; this->type = type; this->name = name; };
  virtual bool isBounded() { return (size>0); };
  virtual void dump() {};
};

class CMSConnection : public CMSBase

{
protected:
  CMSService *service;
  int numChannels;
  int fid, iid;
  
public:
  virtual ChunkID getFixedCopyChunk(int channels, int size, const char *name, CBEType *type) { panic("Not implemented: channel::getFixedCopyChunk"); return 0; };
  virtual ChunkID getVariableCopyChunk(int channels, int minSize, int typSize, int maxSize, const char *name, CBEType *type) { panic("Not implemented: channel::getVariableCopyChunk"); return 0; };
  virtual ChunkID getMapChunk(int channels, int size, const char *name) { panic("Not implemented: channel::getMapChunk"); return 0; };
  virtual void setOption(int option) { panic("Not implemented: channel::setOption"); };
  virtual void optimize() { panic("Not implemented: channel::optimize"); };
  virtual void dump() { panic("Not implemented: channel::dump"); };
  virtual void reset() { panic("Not implemented: channel::reset"); };
  CMSConnection(CMSService *service, int numChannels, int fid, int iid) : CMSBase() 
    { this->numChannels = numChannels; this->service = service;
      this->fid = fid; this->iid = iid; };
  
  virtual CASTStatement *buildClientLocalVars(CASTIdentifier *key) { panic("Not implemented: channel::buildClientLocalVars"); return NULL; };
  virtual CASTStatement *buildClientCall(CASTExpression *target, CASTExpression *env) { panic("Not implemented: channel::buildClientCall"); return NULL; };
  
  /* assignFCCDataToBuffer(channel, chunk, rvalue)

     ACTION:  Copies input data into a fixed-size copy chunk. 
     STORAGE: Provided by the marshaller. 
     SOURCE:  Rvalue. Must remain accessible until the client call or server
              backjump is executed.
     CALL:    Before clientCall and serverBackjump, respectively
     RELEASE: Responsibility of the client */
  
  virtual CASTStatement *assignFCCDataToBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue) { panic("Not implemented: channel::assignFCCDataToBuffer"); return NULL; };
  
  /* assignFCCDataFromBuffer(channel, chunk, rvalue)

     ACTION:  Copies output data from a fixed-size copy chunk. 
     STORAGE: Provided by the client. 
     TARGET:  Lvalue. Must be accessible when the client call or server
              backjump is executed.
     CALL:    Before clientCall and serverBackjump, respectively
     RELEASE: Responsibility of the client */

  virtual CASTStatement *assignFCCDataFromBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue)  { panic("Not implemented: channel::assignFCCDataFromBuffer"); return NULL; };

  /* buildFCCDataSourceExpr(channel, chunk)

     Returns lvalue to the buffer from which the transfer starts. */

  virtual CASTExpression *buildFCCDataSourceExpr(int channel, ChunkID chunk) { panic("Not implemented: channel::buildFCCDataSourceExpr"); return NULL; };

  /* buildFCCDataSourceExpr(channel, chunk)

     Returns lvalue to the buffer where the transfer ends. */
     
  virtual CASTExpression *buildFCCDataTargetExpr(int channel, ChunkID chunk) { panic("Not implemented: channel::buildFCCDataTargetExpr"); return NULL; };

  virtual CASTStatement *assignVCCSizeAndDataToBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue, CASTExpression *size) { panic("Not implemented: channel::assignVCCSizeAndDataToBuffer"); return NULL; };
  virtual CASTStatement *assignVCCDataFromBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue) { panic("Not implemented: channel::assignVCCDataFromBuffer"); return NULL; };
  virtual CASTStatement *assignVCCSizeFromBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue) { panic("Not implemented: channel::assignVCCSizeFromBuffer"); return NULL; };
  virtual CASTStatement *buildVCCServerPrealloc(int channel, ChunkID chunkID, CASTExpression *lvalue) { panic("Not implemented: channel::buildVCCServerPrealloc"); return NULL; };
  virtual CASTStatement *provideVCCTargetBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue, CASTExpression *size) { panic("Not implemented: channel::provideVCCTargetBuffer"); return NULL; };

  virtual CASTStatement *assignFMCFpageToBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue) { panic("Not implemented: channel::assignFMCFpageToBuffer"); return NULL; };
  virtual CASTStatement *assignFMCFpageFromBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue) { panic("Not implemented: channel::assignFMCFpageFromBuffer"); return NULL; };
  virtual CASTExpression *buildFMCFpageSourceExpr(int channel, ChunkID chunk) { panic("Not implemented: channel::buildFMCFpageSourceExpr"); return NULL; };
  virtual CASTExpression *buildFMCFpageTargetExpr(int channel, ChunkID chunk) { panic("Not implemented: channel::buildFMCFpageTargetExpr"); return NULL; };
  
  virtual CASTStatement *buildServerLocalVars(CASTIdentifier *key) { panic("Not implemented: channel::buildServerLocalVars"); return NULL; };
  virtual CASTStatement *buildServerDeclarations(CASTIdentifier *key) { panic("Not implemented: channel::buildServerDeclarations"); return NULL; };
  virtual CASTBase *buildServerWrapper(CASTIdentifier *key, CASTCompoundStatement *compound) { panic("Not implemented: channel::buildServerWrapper"); return NULL; };
  virtual CASTStatement *buildServerBackjump(int channel, CASTExpression *environment) { panic("Not implemented: channel::buildServerBackjump"); return NULL; };
  virtual CASTStatement *buildServerAbort() { panic("Not implemented: channel::buildServerAbort"); return NULL; };
  virtual CASTExpression *buildServerCallerID() { panic("Not implemented: channel::buildServerCallerID"); return NULL; };
  virtual CASTStatement *buildServerMarshalInit() { panic("Not implemented: channel::buildServerMarshalInit"); return NULL; };
  virtual CASTStatement *buildClientInit() { panic("Not implemented: channel::buildClientInit"); return NULL; };
  virtual CASTExpression *buildClientCallSucceeded() { panic("Not implemented: channel::buildClientCallSucceeded"); return NULL; };
  virtual CASTStatement *buildClientResultAssignment(CASTExpression *environment, CASTExpression *defaultValue) { panic("Not implemented: channel::buildClientResultAssignment"); return NULL; };
  virtual CASTStatement *buildClientFinish() { panic("Not implemented: channel::buildClientFinish"); return NULL; };
  
  virtual CASTStatement *buildServerTestStructural() { panic("Not implemented: channel::buildServerTestStructural"); return NULL; };
  virtual CASTStatement *buildServerReplyDeclarations(CASTIdentifier *key) { panic("Not implemented: channel::buildServerReplyDeclarations"); return NULL; };
  virtual CASTStatement *buildServerReply() { panic("Not implemented: channel::buildServerReply"); return NULL; };
};

class CMSService : public CMSBase

{
public:
  CMSService() : CMSBase() {};
  
  virtual CASTStatement *buildServerDeclarations(CASTIdentifier *prefix, int vtableSize, int itableSize, int ktableSize) { panic("Not implemented: service::buildServerDeclarations"); return NULL; };
  virtual CASTStatement *buildServerLoop(CASTIdentifier *prefix, CASTExpression *utableRef, CASTExpression *ktableRef, bool useItable, bool hasKernelMessages) { panic("Not implemented: service::buildServerLoop"); return NULL; };
  virtual CMSConnection *getConnection(int numChannels, int fid, int iid) { panic("Not implemented: service::getConnection"); return NULL; };
  virtual void finalize() { panic("Not implemented: service::finalize()"); };
};

class CMSFactory

{
public:
  CMSFactory() {};
  virtual CMSService *getService() { return new CMSService(); };
  virtual CMSService *getLocalService() { return new CMSService(); };
  virtual CASTStatement *buildIncludes() { return new CASTPreprocInclude(new CASTIdentifier("idl4/idl4.h")); };
  virtual CASTStatement *buildTestIncludes() { return new CASTPreprocInclude(new CASTIdentifier("idl4/test.h")); };
  virtual const char *getInterfaceName() { return "unknown"; };
  virtual const char *getPlatformName() { return "unknown"; };
  CBEWordType *getMachineWordType();
  CBEThreadIDType *getThreadIDType();
  virtual int getThreadIDSize() { panic("No ThreadID size defined"); return 0; };
  virtual int getThreadIDAlignment() { panic("No ThreadID alignment defined"); return 0; };
  virtual void initRootScope(CAoiRootScope *rootScope) { return; };
};

#endif
