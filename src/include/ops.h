#ifndef __ops_h__
#define __ops_h__

#include "ms.h"
#include "cast.h"

#define forAllOps(a, b) \
  do { \
       CBEMarshalOp *item = (a); \
       if (item != NULL) \
         do { \
              b; \
              item = item->next; \
            } while (item != (a)); \
     } while (0)

class CBEMarshalOp : public CBase

{
protected:
  CASTExpression *rvalue;
  CMSConnection *connection;
  CBEParameter *param;
  int channels;
  int flags;
  
public:
  CBEMarshalOp *prev, *next;
  
  CBEMarshalOp(CMSConnection *connection, int channels, CASTExpression *rvalue, CBEParameter *param, int flags) : CBase() 
    { this->prev = this; this->next = this; this->connection = connection; this->channels = channels; this->rvalue = rvalue; this->param = param; this->flags = flags; };
  void add(CBEMarshalOp *newElement);
  
  virtual void buildClientDeclarations(CBEVarSource *varSource) { assert(false); };
  virtual CASTStatement *buildClientMarshal() { assert(false); return NULL; };
  virtual CASTStatement *buildClientUnmarshal() { assert(false); return NULL; };
  virtual void buildServerDeclarations(CBEVarSource *varSource) { assert(false); };
  virtual CASTStatement *buildServerUnmarshal() { assert(false); return NULL; };
  virtual CASTExpression *buildServerArg(CBEParameter *param) { assert(false); return NULL; };
  virtual CASTStatement *buildServerMarshal() { assert(false); return NULL; };
  virtual void buildServerReplyDeclarations(CBEVarSource *varSource) { buildServerDeclarations(varSource); };
  virtual CASTStatement *buildServerReplyMarshal() { assert(false); return NULL; };
  virtual bool isResponsibleFor(CBEParameter *param) { return param == this->param; };
};

class CBEOpSimpleCopy : public CBEMarshalOp

{
  ChunkID chunk;
  
public:
  CBEOpSimpleCopy(CMSConnection *connection, int channels, CASTExpression *rvalue, CBEType *type, int flatSize, const char *name, CBEParameter *param, int flags);

  virtual void buildClientDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildClientMarshal();
  virtual CASTStatement *buildClientUnmarshal();
  virtual void buildServerDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildServerUnmarshal();
  virtual CASTExpression *buildServerArg(CBEParameter *param);
  virtual CASTStatement *buildServerMarshal();
  virtual CASTStatement *buildServerReplyMarshal();
};  

class CBEOpCompatCopy : public CBEMarshalOp

{
  ChunkID chunk;
  CBEType *elementType;
  CASTIdentifier *clientTempVar;
  CASTIdentifier *serverTIDVar, *serverTempVar;
  
public:
  CBEOpCompatCopy(CMSConnection *connection, int channels, CASTExpression *rvalue, CBEType *type, int flatSize, const char *name, CBEParameter *param, int flags);

  virtual void buildClientDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildClientMarshal();
  virtual CASTStatement *buildClientUnmarshal();
  virtual void buildServerDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildServerUnmarshal();
  virtual CASTExpression *buildServerArg(CBEParameter *param);
  virtual CASTStatement *buildServerMarshal();
  virtual void buildServerReplyDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildServerReplyMarshal();
};  

class CBEOpSimpleMap : public CBEMarshalOp

{
  ChunkID chunk;
  
public:
  CBEOpSimpleMap(CMSConnection *connection, int channels, CASTExpression *rvalue, CBEType *type, int flatSize, const char *name, CBEParameter *fpageParam, int flags);

  virtual void buildClientDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildClientMarshal();
  virtual CASTStatement *buildClientUnmarshal();
  virtual void buildServerDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildServerUnmarshal();
  virtual CASTExpression *buildServerArg(CBEParameter *param);
  virtual CASTStatement *buildServerMarshal();
  virtual CASTStatement *buildServerReplyMarshal();
  virtual bool isResponsibleFor(CBEParameter *param);
};  

class CBEOpCopyUntilZero : public CBEMarshalOp

{
  CBEType *elementType;
  ChunkID chunk;
  CASTIdentifier *clientInLengthVar, *clientOutLengthVar;
  CASTIdentifier *serverBufferVar, *serverLengthVar;
  
public:
  CBEOpCopyUntilZero(CMSConnection *connection, int channels, CASTExpression *rvalue, CBEType *type, int sizeBound, const char *name, CBEParameter *param, int flags);

  virtual void buildClientDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildClientMarshal();
  virtual CASTStatement *buildClientUnmarshal();
  virtual void buildServerDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildServerUnmarshal();
  virtual CASTExpression *buildServerArg(CBEParameter *param);
  virtual CASTStatement *buildServerMarshal();
  virtual void buildServerReplyDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildServerReplyMarshal();
};  

class CBEOpCopyArray : public CBEMarshalOp

{
  CBEParameter *lengthParam;
  CBEType *elementType;
  ChunkID chunk;
  CASTIdentifier *serverBufferVar, *serverLengthVar;
  int fixedLength;
  
public:
  CBEOpCopyArray(CMSConnection *connection, int channels, CASTExpression *rvalue, CBEType *type, CBEParameter *lengthParam, const char *name, CBEParameter *param, int flags);
  CBEOpCopyArray(CMSConnection *connection, int channels, CASTExpression *rvalue, CBEType *type, int fixedLength, const char *name, CBEParameter *param, int flags);

  virtual void buildClientDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildClientMarshal();
  virtual CASTStatement *buildClientUnmarshal();
  virtual void buildServerDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildServerUnmarshal();
  virtual CASTExpression *buildServerArg(CBEParameter *param);
  virtual CASTStatement *buildServerMarshal();
  virtual void buildServerReplyDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildServerReplyMarshal();
  virtual bool isResponsibleFor(CBEParameter *param);
};  

class CBEOpCopySubArray : public CBEMarshalOp

{
  CASTExpression *lengthSub, *maxSub, *bufferSub;
  CBEMarshalOp *parentOp;
  CBEType *elementType;
  ChunkID chunk;

public:
  CBEOpCopySubArray(CMSConnection *connection, int channels, CBEMarshalOp *parentOp, CASTExpression *rvalue, CBEType *type, CASTExpression *lengthSub, CASTExpression *maxSub, CASTExpression *bufferSub, const char *name, CBEParameter *param, int flags);

  virtual void buildClientDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildClientMarshal();
  virtual CASTStatement *buildClientUnmarshal();
  virtual void buildServerDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildServerUnmarshal();
  virtual CASTExpression *buildServerArg(CBEParameter *param);
  virtual CASTStatement *buildServerMarshal();
  virtual CASTStatement *buildServerReplyMarshal();
  virtual bool isResponsibleFor(CBEParameter *param);
};

class CBEOpAllocOnServer : public CBEMarshalOp

{
  CASTIdentifier *tempVar;
  CBEType *type;

public:
  CBEOpAllocOnServer(CMSConnection *connection, CBEType *type, CBEParameter *param, int flags);

  virtual void buildClientDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildClientMarshal();
  virtual CASTStatement *buildClientUnmarshal();
  virtual void buildServerDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildServerUnmarshal();
  virtual CASTExpression *buildServerArg(CBEParameter *param);
  virtual CASTStatement *buildServerMarshal();
  virtual void buildServerReplyDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildServerReplyMarshal();
};  

class CBEOpDummy : public CBEMarshalOp

{
public:
  CBEOpDummy(CBEParameter *param);

  virtual void buildClientDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildClientMarshal();
  virtual CASTStatement *buildClientUnmarshal();
  virtual void buildServerDeclarations(CBEVarSource *varSource);
  virtual CASTStatement *buildServerUnmarshal();
  virtual CASTExpression *buildServerArg(CBEParameter *param);
  virtual CASTStatement *buildServerMarshal();
};  

#endif
