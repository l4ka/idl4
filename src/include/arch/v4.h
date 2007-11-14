#ifndef __v4_h__
#define __v4_h__

#include <unistd.h>
#include <assert.h>

#include "globals.h"
#include "aoi.h"
#include "ms.h"

#define FID_SIZE		6 	/* bits */

#define CHUNK_XFER_COPY		(1<<3)	/* is copied in memory */
#define CHUNK_PINNED		(1<<5)	/* cannot be moved (e.g. FIDs) */
#define CHUNK_FORCE_ARRAY       (1<<6)  /* must be declared as an array */

#define POS_NONE		0
#define POS_PRIVILEGES		1

/* V.4 ====================================================================== */

class CMSChunk4 : public CMSChunk

{
public:
  CBEType *contentType;
  int offset;
  int alignment;
  int flags;
  int channels;
  int bufferIndex;
  int specialPos;
  CASTExpression *cachedInExpr;
  CASTExpression *targetBuffer;
  CASTExpression *targetBufferSize;
  
  CMSChunk4(ChunkID chunkID, int size, const char *name, CBEType *type, CBEType *contentType, int channels, int alignment, int flags) : CMSChunk(chunkID, size, name, type) 
    { this->alignment = alignment; this->offset = UNKNOWN; this->flags = flags; this->contentType = contentType; this->bufferIndex = UNKNOWN; this->channels = channels; this->specialPos = POS_NONE; this->cachedInExpr = NULL; this->targetBuffer = NULL; this->targetBufferSize = NULL; };

  virtual CASTDeclarationStatement *buildDeclarationStatement();
  virtual void reset();
  virtual void dump(bool verbose);
};

class CMSService4;

class CMSConnection4 : public CMSConnection

{
protected:
  CMSList *reg_fixed[MAX_CHANNELS];
  CMSList *reg_variable[MAX_CHANNELS];
  CMSList *mem_fixed[MAX_CHANNELS];
  CMSList *mem_variable[MAX_CHANNELS];
  CMSList *strings[MAX_CHANNELS];
  CMSList *fpages[MAX_CHANNELS];
  CMSList *special[MAX_CHANNELS];
  
  int numChannels;
  int bitsPerWord;
  int chunkID;
  int options;
  int channelInitialized;

public:
  CMSConnection4(CMSService4 *service, int numChannels, int fid, int iid, int bitsPerWord);

  virtual ChunkID getFixedCopyChunk(int channels, int size, const char *name, CBEType *type);
  virtual ChunkID getVariableCopyChunk(int channels, int minSize, int typSize, int maxSize, const char *name, CBEType *type);
  virtual ChunkID getMapChunk(int channels, int size, const char *name);
  virtual void optimize();
  virtual void dump();
  virtual void reset();
  virtual void setOption(int option);
  
  virtual CASTStatement *assignFCCDataFromBuffer(int channel, ChunkID chunk, CASTExpression *rvalue);
  virtual CASTStatement *assignFCCDataToBuffer(int channel, ChunkID chunk, CASTExpression *rvalue);
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

  virtual CASTStatement *buildClientLocalVars(CASTIdentifier *key);
  virtual CASTStatement *buildClientCall(CASTExpression *target, CASTExpression *env);
  virtual CASTStatement *buildServerDeclarations(CASTIdentifier *key);
  virtual CASTStatement *buildServerLocalVars(CASTIdentifier *key);
  virtual CASTExpression *buildServerCallerID();
  virtual CASTStatement *buildServerMarshalInit();

  virtual CASTStatement *buildServerBackjump(int channel, CASTExpression *environment);
  virtual CASTStatement *buildServerAbort();
  virtual CASTBase *buildServerWrapper(CASTIdentifier *key, CASTCompoundStatement *compound);
  virtual CASTStatement *buildServerTestStructural();
  virtual CASTStatement *buildClientInit();
  virtual CASTExpression *buildClientCallSucceeded();
  virtual CASTStatement *buildClientResultAssignment(CASTExpression *environment, CASTExpression *defaultValue);
  virtual CASTStatement *buildClientFinish();
  virtual CASTStatement *buildServerReplyDeclarations(CASTIdentifier *key);
  virtual CASTStatement *buildServerReply();

  CASTDeclarationStatement *buildMessageMembers(int channel);
  CASTIdentifier *buildChannelIdentifier(int channel);
  virtual CASTExpression *buildSourceBufferRvalue(int channel);
  virtual CASTExpression *buildTargetBufferRvalue(int channel);
  CASTExpression *buildMemmsgRvalue(int channel, bool isServer);
  CASTAggregateSpecifier *buildMemmsgUnion(bool isServerSide);
  CASTExpression *buildMsgTag(int channel);
  CASTStatement *buildMemMsgSetup(int channel);
  CASTExpression *buildMemMsgSize(int channel);
  CASTDeclarationStatement *buildClientStandardVars();
  CASTDeclarationSpecifier *buildClientMsgBuf();
  CASTIdentifier *buildServerParamID(CASTIdentifier *key);
  CASTStatement *buildServerStandardDecls(CASTIdentifier *key);
  virtual CASTExpression *buildLabelExpr();
  virtual CASTDeclaration *buildWrapperParams(CASTIdentifier *key);
  virtual CASTAttributeSpecifier *buildWrapperAttributes();
  virtual CBEType *getWrapperReturnType();
  CMSChunk4 *findChunk(CMSList *list, ChunkID chunkID);
  void channelDump();
  int layoutMessageRegisters(int channel);
  int alignmentPrio(int sizeInBytes);
  int getMaxBytesRequired();
  int getMaxStringBuffersRequiredOnServer();
  int getMaxUntypedWordsInChannel(int channel);
  int getOutStringItems();
  int getMemMsgDisplacementOnServer();
  CASTStatement *assignStringFromBuffer(int channel, CMSChunk4 *chunk, CASTExpression *lvalue);
  CMSChunk4 *getFirstTypedItem(int channel);
  CASTStatement *buildPositionTest(int channel);
  CASTStatement *buildClientUnmarshalStart();
};

class CMSService4 : public CMSService

{
  CMSList *connections;

public:
  CMSService4() : CMSService() { this->numDwords = 0; this->numStrings = 0; this->connections = new CMSList(); };
  int numDwords, numStrings;
  
  virtual CASTStatement *buildServerDeclarations(CASTIdentifier *prefix, int vtableSize, int itableSize, int ktableSize);
  virtual CMSConnection *buildConnection(int numChannels, int fid, int iid) { return new CMSConnection4(this, numChannels, fid, iid, globals.word_size*8); };
  virtual CMSConnection *getConnection(int numChannels, int fid, int iid);
  virtual CASTStatement *buildServerLoop(CASTIdentifier *prefix, CASTExpression *utableRef, CASTExpression *ktableRef, bool useItable, bool hasKernelMessages);
  virtual void finalize();
};

class CMSFactory4 : public CMSFactory

{
public:
  CMSFactory4() : CMSFactory() 
    { };

  virtual CMSService *getService() { return new CMSService4(); };
  virtual CMSService *getLocalService() { return new CMSService4(); };
  virtual const char *getInterfaceName() { return "V4"; };
  virtual const char *getPlatformName() { return "Generic"; };
  virtual void initRootScope(CAoiRootScope *rootScope);
  virtual int getThreadIDSize() { return globals.word_size; };
  virtual int getThreadIDAlignment() { return globals.word_size; };
};

/* IA32 */

class CMSServiceI4;

class CMSConnectionI4 : public CMSConnection4

{
public:
  CMSConnectionI4(CMSService4 *service, int numChannels, int fid, int iid, int bitsPerWord) : CMSConnection4(service, numChannels, fid, iid, bitsPerWord) {};

  virtual CASTStatement *buildClientCall(CASTExpression *target, CASTExpression *env);
  virtual CASTExpression *buildLabelExpr();
  virtual CASTExpression *buildClientCallSucceeded();
  virtual CASTStatement *buildClientLocalVars(CASTIdentifier *key);
  virtual CASTExpression *buildSourceBufferRvalue(int channel);
  virtual CASTExpression *buildTargetBufferRvalue(int channel);
  virtual CASTStatement *buildClientResultAssignment(CASTExpression *environment, CASTExpression *defaultValue);
  virtual CASTDeclaration *buildWrapperParams(CASTIdentifier *key);
  virtual CBEType *getWrapperReturnType();
  virtual CASTAttributeSpecifier *buildWrapperAttributes();
  virtual CASTStatement *buildServerAbort();
  virtual CASTStatement *buildServerBackjump(int channel, CASTExpression *environment);
  virtual CASTStatement *buildServerReplyDeclarations(CASTIdentifier *key);
  virtual CASTStatement *buildServerReply();
};

class CMSServiceI4 : public CMSService4

{
public:
  CMSServiceI4() : CMSService4() {};

  virtual CMSConnection *buildConnection(int numChannels, int fid, int iid) { return new CMSConnectionI4(this, numChannels, fid, iid, 32); };
};

class CMSFactoryI4 : public CMSFactory4

{
public:
  CMSFactoryI4() : CMSFactory4() {};

  virtual CMSService *getService() { return new CMSServiceI4(); };
  virtual CMSService *getLocalService() { return new CMSServiceI4(); };
  virtual const char *getInterfaceName() { return "V4"; };
  virtual const char *getPlatformName() { return "IA32"; };
};

/* IA64 */

class CMSServiceM4;

class CMSConnectionM4 : public CMSConnection4

{
public:
  CMSConnectionM4(CMSService4 *service, int numChannels, int fid, int iid, int bitsPerWord) : CMSConnection4(service, numChannels, fid, iid, bitsPerWord) {};
};

class CMSServiceM4 : public CMSService4

{
public:
  CMSServiceM4() : CMSService4() {};

  virtual CMSConnection *buildConnection(int numChannels, int fid, int iid) { return new CMSConnectionM4(this, numChannels, fid, iid, 64); };
};

class CMSFactoryM4 : public CMSFactory4

{
public:
  CMSFactoryM4() : CMSFactory4() {};

  virtual CMSService *getService() { return new CMSServiceM4(); };
  virtual CMSService *getLocalService() { return new CMSServiceM4(); };
  virtual const char *getInterfaceName() { return "V4"; };
  virtual const char *getPlatformName() { return "IA64"; };
};
 
#endif
