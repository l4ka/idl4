#ifndef __x0_h__
#define __x0_h__

#include <unistd.h>
#include <assert.h>

#include "globals.h"
#include "ms.h"

#define FID_SIZE		6 	/* bits */

#define PRIO_DOPE		0
#define PRIO_MAP 		1
#define PRIO_PAD		2
#define PRIO_IN_FIXED 		3
#define PRIO_INOUT_FIXED 	4
#define PRIO_OUT_FIXED 		5
#define PRIO_VARIABLE 		6
#define PRIO_ID			9

#define PRIO_IN_STRING		3
#define PRIO_IN_PAD_STRING	4
#define PRIO_INOUT_STRING	5
#define PRIO_INOUT_PAD_STRING	6
#define PRIO_OUT_STRING		7
#define PRIO_OUT_PAD_STRING	8
#define PRIO_TEMPORARY		9

#define SIZEOF_STRINGDOPE	16

/* X.0 ====================================================================== */

/* Generic */

#define CHUNK_ALLOC_CLIENT 	(1<<0)	/* should be allocated on the client side */
#define CHUNK_ALLOC_SERVER 	(1<<1)	/* dito, for the server side */
#define CHUNK_XFER_FIXED	(1<<2)	/* is part of the send/size dope */
#define CHUNK_XFER_COPY		(1<<3)	/* is copied in memory */
#define CHUNK_INIT_ZERO		(1<<4)	/* must be initialized to zero (map pad) */
#define CHUNK_PINNED		(1<<5)	/* cannot be moved (e.g. FIDs) */

#define CHUNK_STANDARD		(CHUNK_ALLOC_CLIENT|CHUNK_ALLOC_SERVER|CHUNK_XFER_FIXED|CHUNK_XFER_COPY)

#define CHUNK_NOALIGN	1
#define CHUNK_ALIGN4	4
#define CHUNK_ALIGN8	8

class CMSChunkX : public CMSChunk

{
  CASTExpression *cachedInExpr[2];
  CASTExpression *cachedOutExpr[2];
  
public:
  CASTExpression *targetBuffer, *targetBufferSize;
  CBEType *contentType;
  int channels;
  int flags;
  int alignment;
  int sizeBound;
  int requestedIndex;
  int clientOffset, serverOffset;
  int clientIndex, serverIndex;
  CASTIdentifier *temporaryBuffer;
  
  CMSChunkX(ChunkID chunkID, const char *name, int size, int sizeBound, int alignment, CBEType *type, CBEType *contentType, int channels, int flags, int requestedIndex = 0);
  virtual CMSChunkX *findSubchunk(ChunkID chunkID);
  virtual void setClientPosition(int clientOffset, int clientIndex);
  virtual void setServerPosition(int serverOffset, int serverIndex);
  virtual void dump(bool showDetails);
  virtual void reset();
  virtual void setCachedInput(int nr, CASTExpression *rvalue);
  virtual void setCachedOutput(int nr, CASTExpression *lvalue);
  virtual CASTExpression *getCachedInput(int nr);
  virtual CASTExpression *getCachedOutput(int nr);
  virtual bool isShared() { return false; };

  virtual CASTDeclarationStatement *buildDeclarationStatement();
  virtual CASTDeclarationStatement *buildRegisterBuffer(int channel, CASTExpression *reg);
};

class CMSSharedChunkX : public CMSChunkX

{
public: 
  CMSChunkX *chunk1, *chunk2;

  CMSSharedChunkX(CMSChunkX *chunk1, CMSChunkX *chunk2);
  virtual CMSChunkX *findSubchunk(ChunkID chunkID);
  virtual void setClientPosition(int clientOffset, int clientIndex);
  virtual void setServerPosition(int serverOffset, int serverIndex);
  virtual void dump(bool showDetails);
  virtual void reset();
  virtual void setCachedInput(int nr, CASTExpression *rvalue);
  virtual void setCachedOutput(int nr, CASTExpression *lvalue);
  virtual CASTExpression *getCachedInput(int nr);
  virtual CASTExpression *getCachedOutput(int nr);
  virtual bool isShared() { return true; };

  virtual CASTDeclarationStatement *buildDeclarationStatement();
  virtual CASTDeclarationStatement *buildRegisterBuffer(int channel, CASTExpression *reg);
};

class CMSServiceX;

class CMSConnectionX : public CMSConnection

{
protected:
  CMSList *dopes[MAX_CHANNELS];
  CMSList *registers[MAX_CHANNELS];
  CMSList *dwords[MAX_CHANNELS];
  CMSList *vars[MAX_CHANNELS];
  CMSList *strings[MAX_CHANNELS];

  CMSServiceX *service;
  int numRegs, numChannels;
  int bitsPerWord;
  int chunkID;
  int options;
  int channelInitialized;
  bool finalized;

  virtual bool isWorthString(int minSize, int typSize, int maxSize);
  virtual int calculatePriority(int channels);
  
public:
  virtual ChunkID getFixedCopyChunk(int channels, int size, const char *name, CBEType *type);
  virtual ChunkID getVariableCopyChunk(int channels, int minSize, int typSize, int maxSize, const char *name, CBEType *type);
  virtual ChunkID getMapChunk(int channels, int size, const char *name);
  virtual void optimize();
  virtual void dump();
  virtual void reset();
  virtual void setOption(int option);
  virtual void finalize(int strdopeOffsetInBytes);
  CMSConnectionX(CMSServiceX *service, int numChannels, int fid, int iid, int numRegs, int bitsPerWord);

  virtual CASTStatement *buildClientLocalVars(CASTIdentifier *key);
  virtual CASTStatement *buildClientCall(CASTExpression *target, CASTExpression *env);

  virtual CASTStatement *assignFCCDataToBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue);
  virtual CASTStatement *assignFCCDataFromBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue);
  virtual CASTExpression *buildFCCDataSourceExpr(int channel, ChunkID chunkID);
  virtual CASTExpression *buildFCCDataTargetExpr(int channel, ChunkID chunkID);

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
  virtual CASTStatement *buildServerDeclarations(CASTIdentifier *key);
  virtual CASTBase *buildServerWrapper(CASTIdentifier *key, CASTCompoundStatement *compound);
  virtual CASTStatement *buildServerBackjump(int channel, CASTExpression *environment);
  virtual CASTStatement *buildServerAbort();
  virtual CASTExpression *buildServerCallerID();
  virtual CASTStatement *buildServerMarshalInit();
  virtual CASTDeclaration *buildWrapperParams(CASTIdentifier *key);

  virtual CASTStatement *buildServerTestStructural();
  virtual CASTStatement *buildClientInit();
  virtual CASTExpression *buildClientCallSucceeded();
  virtual CASTStatement *buildClientResultAssignment(CASTExpression *environment, CASTExpression *defaultValue);
  virtual CASTStatement *buildClientFinish();
  virtual CASTStatement *buildServerReplyDeclarations(CASTIdentifier *key);
  virtual CASTStatement *buildServerReply();

  CASTDeclarationStatement *buildMessageMembers(int channel, bool isServer);
  CASTIdentifier *buildChannelIdentifier(int channel);
  virtual CASTExpression *buildSourceBufferRvalue(int channel);
  virtual CASTExpression *buildTargetBufferRvalue(int channel);
  CASTExpression *buildClientSizeDope();
  CASTExpression *buildSendDope(int channel);
  bool isShortIPC(int channel);
  bool hasOutFpages();
  bool hasFpagesInChannel(int channel);
  bool hasLongOutIPC();
  bool hasStringsInChannel(int channel);
  int getMaxBytesRequired(bool onServer);
  int getMaxStringBuffersRequiredOnServer();
  int getMaxStringsAcceptedByServer();
  int getVarbufSize(int channel);
  void channelDump();
  CMSChunkX *findChunk(CMSList *list, ChunkID chunkID);
  CASTStatement *buildServerDopeAssignment();
  CASTStatement *buildClientDopeAssignment(CASTExpression *env);
  CASTStatement *buildClientLocalVarsIndep();
  CASTStatement *buildPositionTest(int channel, CMSChunkX *chunk);
  void analyzeChannel(int channel, bool serverSide, bool includeVariables, int *firstXferByte, int *lastXferByte, int *firstXferString, int *numXferStrings);
  virtual void addServerDopeExtensions();
  virtual const char *getIPCBindingName(bool sendOnly);
  virtual CASTExpression *buildStackTopExpression();
  virtual CASTExpression *buildCallerExpression();
  virtual CASTExpression *buildRegisterVar(int index);
};

class CMSServiceX : public CMSService

{
protected:
  CMSList *connections;
  int numDwords, numStrings;

public:
  CMSServiceX() : CMSService() { this->numDwords = 0; this->numStrings = 0; this->connections = new CMSList(); };
  
  virtual CMSConnection *buildConnection(int numChannels, int fid, int iid) { return new CMSConnectionX(this, numChannels, fid, iid, 3, 32); };
  virtual CMSConnection *getConnection(int numChannels, int fid, int iid);
  virtual CASTStatement *buildServerDeclarations(CASTIdentifier *prefix, int vtableSize, int itableSize, int ktableSize);
  virtual CASTStatement *buildServerLoop(CASTIdentifier *prefix, CASTExpression *utableRef, CASTExpression *ktableRef, bool useItable, bool hasKernelMessages);
  virtual void finalize();
  virtual const char *getShortRegisterName(int no);
  virtual const char *getLongRegisterName(int no);
  virtual CASTAsmConstraint *getThreadidInConstraints(CASTExpression *rvalue);
  virtual int getServerLocalVars() { return 0; };
  
  int getServerSizeDope(int byteOffset);
};

class CMSFactoryX : public CMSFactory

{
public:
  CMSFactoryX() : CMSFactory() 
    { };

  virtual CMSService *getService() { return new CMSServiceX(); };
  virtual CMSService *getLocalService() { return new CMSServiceX(); };
  virtual const char *getInterfaceName() { return "X0"; };
  virtual const char *getPlatformName() { return "Generic"; };
  virtual int getThreadIDSize() { return 4; };
  virtual int getThreadIDAlignment() { return 4; };
};
 
/* IA32 */

class CMSConnectionIX : public CMSConnectionX

{
public:
  CMSConnectionIX(CMSServiceX *service, int numChannels, int fid, int iid, int numRegs, int bitsPerWord) : CMSConnectionX(service, numChannels, fid, iid, numRegs, bitsPerWord)
    { };

  virtual CASTStatement *buildClientCall(CASTExpression *target, CASTExpression *env);
  virtual CASTStatement *buildClientLocalVars(CASTIdentifier *key);
  virtual CASTStatement *buildServerDeclarations(CASTIdentifier *key);
  virtual CASTStatement *buildServerBackjump(int channel, CASTExpression *environment);
  virtual CASTStatement *buildServerAbort();
  virtual CASTBase *buildServerWrapper(CASTIdentifier *key, CASTCompoundStatement *compound);
  virtual CASTDeclaration *buildWrapperParams(CASTIdentifier *key);
  virtual CASTExpression *buildRegisterVar(int index);
  virtual CASTExpression *buildSourceBufferRvalue(int channel);
  virtual CASTExpression *buildTargetBufferRvalue(int channel);
  virtual CASTExpression *buildClientCallSucceeded();
  virtual CASTStatement *buildClientResultAssignment(CASTExpression *environment, CASTExpression *defaultValue);
  virtual CASTStatement *buildServerReplyDeclarations(CASTIdentifier *key);
  virtual CASTStatement *buildServerReply();
  virtual void addServerDopeExtensions();
  virtual void optimize();
};

class CMSServiceIX : public CMSServiceX

{
public:
  CMSServiceIX() : CMSServiceX() {};
  
  virtual CMSConnection *buildConnection(int numChannels, int fid, int iid) { return new CMSConnectionIX(this, numChannels, fid, iid, 3, 32); };
  virtual CASTStatement *buildServerLoop(CASTIdentifier *prefix, CASTExpression *utableRef, CASTExpression *ktableRef, bool useItable, bool hasKernelMessages);
  virtual int getServerLocalVars() { return 2; };
};

class CMSConnectionIXL : public CMSConnectionIX

{
public:
  CMSConnectionIXL(CMSServiceX *service, int numChannels, int fid, int iid, int numRegs, int bitsPerWord) : CMSConnectionIX(service, numChannels, fid, iid, numRegs, bitsPerWord)
    { };

  virtual CASTExpression *buildFCCDataSourceExpr(int channel, ChunkID chunk);
  virtual CASTExpression *buildFCCDataTargetExpr(int channel, ChunkID chunk);

  virtual CASTStatement *buildClientCall(CASTExpression *target, CASTExpression *env);
  virtual CASTStatement *buildClientLocalVars(CASTIdentifier *key);
  virtual CASTStatement *buildServerDeclarations(CASTIdentifier *key);
  virtual CASTBase *buildServerWrapper(CASTIdentifier *key, CASTCompoundStatement *compound);
  virtual CASTExpression *buildServerCallerID();
  virtual CASTStatement *buildServerBackjump();
  virtual CASTExpression *buildStackTopExpression();
  virtual CASTExpression *buildCallerExpression();
};

class CMSServiceIXL : public CMSServiceIX

{
public:
  CMSServiceIXL() : CMSServiceIX() {};
  
  virtual CMSConnection *buildConnection(int numChannels, int fid, int iid) { return new CMSConnectionIXL(this, numChannels, fid, iid, 3, 32); };
  virtual CASTStatement *buildServerLoop(CASTIdentifier *prefix, CASTExpression *utableRef, CASTExpression *ktableRef, bool useItable, bool hasKernelMessages);
};

class CMSFactoryIX : public CMSFactoryX

{
public:
  CMSFactoryIX() : CMSFactoryX() {};
  virtual CMSService *getService() { return new CMSServiceIX(); };
  virtual CMSService *getLocalService();
  virtual const char *getInterfaceName() { return "X0"; };
  virtual const char *getPlatformName() { return "IA32"; };
};

/* ARM */

class CMSFactoryAX : public CMSFactory

{
public:
  CMSFactoryAX() {};
};

#endif
