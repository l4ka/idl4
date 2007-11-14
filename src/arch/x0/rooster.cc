#include "cast.h"
#include "arch/x0.h"

CMSChunkX::CMSChunkX(ChunkID chunkID, const char *name, int size, int sizeBound, int alignment, CBEType *type, CBEType *contentType, int channels, int flags, int requestedIndex) : CMSChunk(chunkID, size, name, type) 

{
  for (int nr=0;nr<2;nr++)
    {
      this->cachedInExpr[nr] = NULL; 
      this->cachedOutExpr[nr] = NULL; 
    }  
  this->channels = channels; 
  this->flags = flags;
  this->alignment = alignment; 
  this->clientOffset = UNKNOWN;
  this->serverOffset = UNKNOWN;
  this->clientIndex = UNKNOWN;
  this->serverIndex = UNKNOWN;
  this->sizeBound = sizeBound;
  this->contentType = contentType;
  this->temporaryBuffer = NULL;
  this->requestedIndex = requestedIndex;
  this->targetBuffer = NULL;
  this->targetBufferSize = NULL;
}

CMSSharedChunkX::CMSSharedChunkX(CMSChunkX *chunk1, CMSChunkX *chunk2) : CMSChunkX(UNKNOWN, "shared", 0, 0, 0, NULL, NULL, 0, 0)

{
  this->chunk1 = chunk1;
  this->chunk2 = chunk2;
  this->alignment = max(chunk1->alignment, chunk2->alignment);
  this->size = chunk1->size + chunk2->size;
  this->sizeBound = chunk1->sizeBound + chunk2->sizeBound;
  this->flags = chunk1->flags | chunk2->flags;
}

void CMSSharedChunkX::reset()

{
  chunk1->reset();
  chunk2->reset();
}

CASTDeclarationStatement *CMSSharedChunkX::buildDeclarationStatement()

{
  CASTDeclarationStatement *result = NULL;
  
  addTo(result, chunk1->buildDeclarationStatement());
  addTo(result, chunk2->buildDeclarationStatement());
  
  return result;
}

CASTDeclarationStatement *CMSSharedChunkX::buildRegisterBuffer(int channel, CASTExpression *reg)

{
  CASTDeclarationStatement *result = NULL;
  
  addTo(result, chunk1->buildRegisterBuffer(channel, reg));
  addTo(result, chunk2->buildRegisterBuffer(channel, reg));
  
  return result;
}

int CMSConnectionX::getMaxBytesRequired(bool onServer)

{
  int endOfBuffer = 0;
  
  for (int i=0;i<numChannels;i++)
    {
      forAllMS(registers[i],
        {
          CMSChunkX* chunk = (CMSChunkX*)item;
          endOfBuffer = max(endOfBuffer, ((onServer) ? chunk->serverOffset : chunk->clientOffset) + chunk->size);
        }  
      );  
      forAllMS(dwords[i],
        {
          CMSChunkX* chunk = (CMSChunkX*)item;
          endOfBuffer = max(endOfBuffer, ((onServer) ? chunk->serverOffset : chunk->clientOffset) + chunk->size);
        }  
      );  
      
      /* If strings are used, the message buffer needs to fit in */
      
      if (!strings[i]->isEmpty() && (endOfBuffer<((3+numRegs)*4)))
        endOfBuffer = (3+numRegs)*4;
    }    
    
  return endOfBuffer;
}

int CMSConnectionX::getMaxStringsAcceptedByServer()

{
  int stringsAccepted = 0;
  
  assert(finalized);
  
  forAllMS(strings[CHANNEL_IN], 
    {
      CMSChunkX *chunk = (CMSChunkX*)item;
      if (chunk->flags&CHUNK_XFER_COPY)
        stringsAccepted++;
    }  
  );  
    
  return stringsAccepted;
}

int CMSConnectionX::getMaxStringBuffersRequiredOnServer()

{
  int stringsRequired = 0;

  for (int i=0; i<numChannels; i++)
    {
      int thisChannel = 0;
      forAllMS(strings[i], thisChannel++);
      if (thisChannel>stringsRequired)
        stringsRequired = thisChannel;
    }  

  return stringsRequired;
}

void CMSConnectionX::analyzeChannel(int channel, bool serverSide, bool includeVariables, int *firstXferByte, int *lastXferByte, int *firstXferString, int *numXferStrings)

{
  *firstXferByte = UNKNOWN;
  *lastXferByte = UNKNOWN;

  forAllMS(dwords[channel],
    {
      CMSChunkX *chunk = (CMSChunkX*)item;
      int offset = (serverSide) ? chunk->serverOffset : chunk->clientOffset;
      if ((chunk->flags & CHUNK_XFER_COPY) &&
          (includeVariables || (chunk->flags & CHUNK_XFER_FIXED)))
        {
          int lastByte = offset + chunk->size - 1;
          if ((*firstXferByte==UNKNOWN) || (offset < *firstXferByte))
            *firstXferByte = offset;
          if ((*lastXferByte==UNKNOWN) || (lastByte > *lastXferByte))
            *lastXferByte = lastByte;
        }
    }    
  );  
    
  *firstXferString = UNKNOWN;
  *numXferStrings = 0;
  
  forAllMS(strings[channel],
    {
      CMSChunkX *chunk = (CMSChunkX*)item;
      int offset = (serverSide) ? chunk->serverOffset : chunk->clientOffset;
      if ((chunk->flags & CHUNK_XFER_COPY) &&
          (includeVariables || (chunk->flags & CHUNK_XFER_FIXED)))
        {
          if ((*firstXferString==UNKNOWN) || (*firstXferString>offset))
            *firstXferString = offset;
          (*numXferStrings)++;
        }
    }    
  );
}

CASTExpression *CMSConnectionX::buildClientSizeDope()

{
  int stringDopePos = UNKNOWN;
  int maxStrings = 0;
  int maxBytes = 0;

  for (int i=0;i<numChannels;i++)
    {
      int firstXferByte, lastXferByte, firstXferString, numXferStrings;
      analyzeChannel(i, false, true, &firstXferByte, &lastXferByte, &firstXferString, &numXferStrings);

      assert(firstXferByte==UNKNOWN || firstXferByte==(3+numRegs)*4);
      if (stringDopePos==UNKNOWN)
        stringDopePos = firstXferString;
        
      assert(stringDopePos==UNKNOWN || firstXferString==UNKNOWN || stringDopePos==firstXferString);
      
      if (i!=CHANNEL_IN)
        {
          if (numXferStrings>maxStrings)
            maxStrings = numXferStrings;
        
          int bytesInThisChannel = (firstXferByte==UNKNOWN) ? 0 : (lastXferByte - firstXferByte + 1);
          if (bytesInThisChannel>maxBytes)
            maxBytes = bytesInThisChannel;
        }    
    }
    
  assert(stringDopePos==UNKNOWN || stringDopePos>=maxBytes);
  if ((stringDopePos!=UNKNOWN) && (maxBytes < stringDopePos))
    maxBytes = stringDopePos - (3+numRegs)*4;
  
  return new CASTHexConstant(((numRegs + ((maxBytes+3)/4))<<13) + (maxStrings<<8));
}

CASTExpression *CMSConnectionX::buildSendDope(int channel)

{
  CASTExpression *result = NULL;

  int firstXferByte, lastXferByte, firstXferString, numXferStrings;
  analyzeChannel(channel, (channel!=CHANNEL_IN), false, &firstXferByte, &lastXferByte, &firstXferString, &numXferStrings);

  int nDwords = (firstXferByte==UNKNOWN) ? 0 : (((lastXferByte-firstXferByte+1)+3)/4);
  int fixedPart = ((nDwords+numRegs)<<13) + (numXferStrings<<8);
  result = new CASTHexConstant(fixedPart);
  
  if (!vars[channel]->isEmpty())
    result = new CASTBinaryOp("+",
               result,
               new CASTBinaryOp("<<",
                 new CASTIdentifier("_varidx"),
                 new CASTIntegerConstant(11)));
 
  return result;
}

CASTDeclarationStatement *buildMember(CMSChunkX *item, int *currentPosition, int *spacerIndex, bool isServer)

{
  int chunkPosition = (isServer) ? item->serverOffset : item->clientOffset;
  int padding = chunkPosition - *currentPosition;
  CASTDeclarationStatement *result = NULL;

  if ((isServer && !(item->flags&CHUNK_ALLOC_SERVER)) || (!isServer && !(item->flags&CHUNK_ALLOC_CLIENT)))
    return NULL;
      
  if (padding)
    addTo(result, new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTTypeSpecifier(new CASTIdentifier("char")),
        new CASTDeclarator(
          new CASTIdentifier(mprintf("_spacer%d", ++*spacerIndex)),
          NULL,
          new CASTIntegerConstant(padding))))
    );      
    
  addTo(result, item->buildDeclarationStatement());
  *currentPosition = chunkPosition + item->size;
  
  return result;
}

CASTDeclarationStatement *CMSConnectionX::buildMessageMembers(int channel, bool isServer)

{
  CASTDeclarationStatement *dwordMembers = NULL;
  int currentPosition = 0;
  int spacerIndex = 0;
  
  if (isServer) 
    currentPosition -= 4*service->getServerLocalVars();

  forAllMS(dopes[channel], addTo(dwordMembers, buildMember((CMSChunkX*)item, &currentPosition, &spacerIndex, isServer)));
  forAllMS(registers[channel], addTo(dwordMembers, buildMember((CMSChunkX*)item, &currentPosition, &spacerIndex, isServer)));
  forAllMS(dwords[channel], addTo(dwordMembers, buildMember((CMSChunkX*)item, &currentPosition, &spacerIndex, isServer)));
  forAllMS(strings[channel], addTo(dwordMembers, buildMember((CMSChunkX*)item, &currentPosition, &spacerIndex, isServer)));

  return dwordMembers;  
}

bool CMSConnectionX::isShortIPC(int channel)

{
  forAllMS(dwords[channel], if (((CMSChunkX*)item)->flags & CHUNK_XFER_COPY) return false);
  forAllMS(strings[channel], if (((CMSChunkX*)item)->flags & CHUNK_XFER_COPY) return false);
  forAllMS(vars[channel], if (((CMSChunkX*)item)->flags & CHUNK_XFER_COPY) return false);
  
  return true;
}  

bool CMSConnectionX::hasOutFpages()

{
  for (int i=CHANNEL_OUT; i<numChannels; i++)
    if (hasFpagesInChannel(i))
      return true;
      
  return false;
}

bool CMSConnectionX::hasStringsInChannel(int channel)

{
  forAllMS(strings[channel], 
    if (((CMSChunkX*)item)->flags & CHUNK_XFER_COPY)
      return true
  );
  
  return false;
}

bool CMSConnectionX::hasFpagesInChannel(int channel)

{
  return (registers[channel]->getFirstElementWithPrio(PRIO_MAP) ||
          dwords[channel]->getFirstElementWithPrio(PRIO_MAP));
}

bool CMSConnectionX::hasLongOutIPC()

{
  for (int i=CHANNEL_OUT; i<numChannels; i++)
    if (!isShortIPC(i))
      return true;
      
  return false;
}

CASTStatement *CMSConnectionX::buildPositionTest(int channel, CMSChunkX *chunk)

{
  CASTExpression *bufferName;
  int assumedPos = chunk->serverOffset + 4*service->getServerLocalVars();

  if (!(chunk->flags&CHUNK_ALLOC_SERVER))
    return NULL;
    
  if (chunk->isShared())
    {
      CMSSharedChunkX *schunk = (CMSSharedChunkX*)chunk;
      CASTStatement *result = NULL;
      
      addTo(result, buildPositionTest(channel, schunk->chunk1));
      addTo(result, buildPositionTest(channel, schunk->chunk2));
      return result;
    }  

  if (channel==CHANNEL_IN)
    bufferName = buildTargetBufferRvalue(CHANNEL_IN);
    else bufferName = buildSourceBufferRvalue(channel);

  CASTExpression *actualPosition = 
    new CASTBinaryOp("-",
      new CASTUnaryOp("(int)",
        new CASTBrackets(
          new CASTUnaryOp("&",
            new CASTBinaryOp(".",
              bufferName,
              new CASTIdentifier(chunk->name))))),
      new CASTUnaryOp("(int)",
        new CASTBrackets(
          new CASTUnaryOp("&",
            bufferName->clone())))
    );

  return new CASTIfStatement(
    new CASTBinaryOp("!=",
      actualPosition,
      new CASTIntegerConstant(assumedPos)),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(
          new CASTStringConstant(false, mprintf("Alignment error in channel %d: %s is at %%d (should be %d)", 
                                                channel, chunk->name, assumedPos)),
          actualPosition->clone()
        )))
  ); 
}

CASTStatement *CMSConnectionX::buildServerTestStructural()

{
  CASTStatement *result = NULL;
  
  for (int i=0;i<numChannels;i++)
    {
      forAllMS(registers[i], addTo(result, buildPositionTest(i, (CMSChunkX*)item)));
      forAllMS(dwords[i], addTo(result, buildPositionTest(i, (CMSChunkX*)item)));
      forAllMS(strings[i], addTo(result, buildPositionTest(i, (CMSChunkX*)item)));
    }  
      
  return result; 
}
