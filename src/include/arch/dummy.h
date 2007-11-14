#ifndef __arch_dummy_h__
#define __arch_dummy_h__

#include <unistd.h>
#include <assert.h>

#include "globals.h"
#include "aoi.h"
#include "ms.h"

class CMSConnectionDummy : public CMSConnection

{
  int nextChunk;
  
public:
  virtual ChunkID getFixedCopyChunk(int channels, int size, const char *name, CBEType *type);
  virtual ChunkID getVariableCopyChunk(int channels, int minSize, int typSize, int maxSize, const char *name, CBEType *type);
  virtual ChunkID getMapChunk(int channels, int size, const char *name);
  virtual void setOption(int option);
  virtual void optimize();
  virtual void dump();
  virtual void reset();
  CMSConnectionDummy(CMSService *service, int fid, int iid);

  virtual CASTStatement *buildClientLocalVars(CASTIdentifier *key);
  virtual CASTStatement *buildClientCall(CASTExpression *target, CASTExpression *env);

  virtual CASTStatement *assignFCCDataToBuffer(int channel, ChunkID chunk, CASTExpression *rvalue);
  virtual CASTStatement *assignFCCDataFromBuffer(int channel, ChunkID chunk, CASTExpression *rvalue);
  virtual CASTExpression *buildFCCDataSourceExpr(int channel, ChunkID chunk);
  virtual CASTExpression *buildFCCDataTargetExpr(int channel, ChunkID chunk);

  virtual CASTStatement *assignVCCSizeAndDataToBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue, CASTExpression *size);
  virtual CASTStatement *assignVCCDataFromBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue);
  virtual CASTStatement *assignVCCSizeFromBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue);
  virtual CASTStatement *buildVCCServerPrealloc(int channel, ChunkID chunkID, CASTExpression *lvalue);
  virtual CASTStatement *provideVCCTargetBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue, CASTExpression *size);

  virtual CASTStatement *assignFMCFpageToBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue);
  virtual CASTStatement *assignFMCFpageFromBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue);
  virtual CASTExpression *buildFMCFpageSourceExpr(int channel, ChunkID chunk);
  virtual CASTExpression *buildFMCFpageTargetExpr(int channel, ChunkID chunk);

  virtual CASTStatement *buildServerLocalVars(CASTIdentifier *key);
  virtual CASTStatement *buildServerBackjump(int channel, CASTExpression *environment);
  virtual CASTStatement *buildServerAbort();

  virtual CASTStatement *buildServerDeclarations(CASTIdentifier *key);
  virtual CASTExpression *buildServerCallerID();
  virtual CASTBase *buildServerWrapper(CASTIdentifier *key, CASTCompoundStatement *compound);
  virtual CASTStatement *buildServerMarshalInit();

  virtual CASTStatement *buildClientInit();
  virtual CASTExpression *buildClientCallSucceeded();
  virtual CASTStatement *buildClientResultAssignment(CASTExpression *environment, CASTExpression *defaultValue);
  virtual CASTStatement *buildClientFinish();

  virtual CASTStatement *buildServerTestStructural();
  virtual CASTStatement *buildServerReplyDeclarations(CASTIdentifier *key);
  virtual CASTStatement *buildServerReply();
};

class CMSServiceDummy : public CMSService

{
public:
  CMSServiceDummy() : CMSService() {};
  
  virtual CMSConnection *getConnection(int numChannels, int fid, int iid) { return new CMSConnectionDummy(this, fid, iid); };
  virtual CASTStatement *buildServerLoop(CASTIdentifier *prefix, CASTExpression *utableRef, CASTExpression *ktableRef, bool useItable, bool hasKernelMessages);
  virtual CASTStatement *buildServerDeclarations(CASTIdentifier *prefix, int vtableSize, int itableSize, int ktableSize);
  virtual void finalize();
};

class CMSFactoryDummy : public CMSFactory

{
public:
  CMSFactoryDummy() { };
  virtual CMSService *getService() { return new CMSServiceDummy(); };
  virtual CMSService *getLocalService() { return new CMSServiceDummy(); };
  virtual const char *getInterfaceName() { return "Generic"; };
  virtual const char *getPlatformName() { return "Dummy"; };
  virtual int getThreadIDSize() { return globals.word_size; };
  virtual int getThreadIDAlignment() { return globals.word_size; };
  virtual CASTStatement *buildIncludes();
  virtual CASTStatement *buildTestIncludes();
};

#endif
