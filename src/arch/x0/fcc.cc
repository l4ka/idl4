#include "cast.h"
#include "arch/x0.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

ChunkID CMSConnectionX::getFixedCopyChunk(int channels, int size, const char *name, CBEType *type)

{
  int thisChunk = ++chunkID;
  int prio = calculatePriority(channels);

  assert(type!=NULL);
  dprintln("getFCC channels=%d size=%d name=%s -> chunkID %d", channels, size, name, thisChunk);
  
  for (int i=0;i<numChannels;i++)
    if (channels&(1<<i))
      {
        if (!(channelInitialized & CHANNEL_MASK(i)))
          {
            CMSChunkX *eidChunk = new CMSChunkX(0, name, size, size, type->getAlignment(), type, NULL, 0, CHUNK_ALLOC_CLIENT|CHUNK_ALLOC_SERVER|CHUNK_XFER_FIXED|CHUNK_XFER_COPY|CHUNK_PINNED, (numRegs) ? (numRegs - 1) : 0);
            if (numRegs)
              registers[i]->addWithPrio(eidChunk, PRIO_ID);
              else dwords[i]->addWithPrio(eidChunk, PRIO_DOPE);  
            channelInitialized |= CHANNEL_MASK(i);
          } else dwords[i]->addWithPrio(new CMSChunkX(thisChunk, name, size, size, type->getAlignment(), type, type, channels, CHUNK_STANDARD), prio);
      }    
    
  return thisChunk;  
}

CASTStatement *CMSConnectionX::assignFCCDataToBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue)

{
  CMSChunkX *chunk;

  /* Register values will not produce a statement immediately; instead, they
     will be included in the client call (asm statement) */

  chunk = findChunk(registers[channel], chunkID);
  if (chunk)
    {
      chunk->setCachedInput(0, rvalue);
      return NULL;    
    }

  /* Dword values can be assigned directly */
  
  chunk = findChunk(dwords[channel], chunkID);
  if (chunk)
    {
      /* If the chunk is an INOUT value and stays where it is, we do not
         need to reassign it */

      if ((channel==CHANNEL_OUT) && HAS_IN(chunk->channels) && HAS_OUT(chunk->channels))
        {
          CMSChunkX *inChunk = findChunk(dwords[CHANNEL_IN], chunkID);
          CMSChunkX *outChunk = findChunk(dwords[CHANNEL_OUT], chunkID);
          
          assert(inChunk && outChunk);
          if (inChunk->serverOffset == outChunk->serverOffset)
            if (rvalue->equals(buildFCCDataTargetExpr(CHANNEL_IN, chunkID)))
              return NULL; 
        }      

      /* In all other cases, we may need to reassign. Note that source
         and target expression may be equal; this happens when the source
         was passed to the procedure by reference. */

      CASTExpression *target = new CASTBinaryOp(".",
        buildSourceBufferRvalue(channel),
        new CASTIdentifier(chunk->name)
      );

      if (!target->equals(rvalue))
        return chunk->type->buildAssignment(target, rvalue);
        else return NULL;
    }

  panic("Cannot assign FCC data to buffer (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}      

CASTStatement *CMSConnectionX::assignFCCDataFromBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue)

{
  CMSChunkX *chunk;

  /* Register values will not produce a statement immediately; instead, they
     will be included in the client call (asm statement) */

  chunk = findChunk(registers[channel], chunkID);
  if (chunk)
    {
      chunk->setCachedOutput(0, rvalue);
      return NULL;    
    }

  /* Dword values can be assigned directly */
  
  chunk = findChunk(dwords[channel], chunkID);
  if (chunk)
    {
      return chunk->type->buildAssignment(
        rvalue, 
        new CASTBinaryOp(".",
          buildTargetBufferRvalue(channel),
          new CASTIdentifier(chunk->name))
      );
    }

  panic("Cannot assign FCC data from buffer (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}      

CASTExpression *CMSConnectionX::buildFCCDataSourceExpr(int channel, ChunkID chunkID)

{
  CMSChunkX *chunk;

  chunk = findChunk(dwords[channel], chunkID);
  if (chunk)
    {
      return new CASTBinaryOp(".",
        buildSourceBufferRvalue(channel),
        new CASTIdentifier(chunk->name)
      );    
    }

  /* On the server, register targets are actually in the buffer */
  
  if (channel!=CHANNEL_IN)
    {
      chunk = findChunk(registers[channel], chunkID);
      if (chunk)
        {
          if (chunk->temporaryBuffer)
            {
              CASTExpression *result = chunk->temporaryBuffer->clone();
              
              if (!chunk->type->isScalar() && HAS_IN(chunk->channels))
                result = new CASTBinaryOp(".", 
                  result,
                  new CASTIdentifier("typed")
                );
                
              return result;
            }

        return new CASTIdentifier(chunk->name);
      }
    }    

  panic("Cannot build FCC data inbuf expression (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}      

CASTExpression *CMSConnectionX::buildFCCDataTargetExpr(int channel, ChunkID chunkID)

{
  CMSChunkX *chunk;

  /* Dwords is easy. We just give back the element in the buffer */

  chunk = findChunk(dwords[channel], chunkID);
  if (chunk)
    {
      return new CASTBinaryOp(".",
        buildTargetBufferRvalue(channel),
        new CASTIdentifier(chunk->name)
      );    
    }
 
  /* On the server, register targets are passed via arguments */
  
  if (channel==CHANNEL_IN)
    {
      chunk = findChunk(registers[channel], chunkID);
      if (chunk)
        {
          if (chunk->temporaryBuffer)
            {
              CASTExpression *result = chunk->temporaryBuffer->clone();
              
              if (!chunk->type->isScalar() && HAS_IN(chunk->channels))
                result = new CASTBinaryOp(".", 
                  result,
                  new CASTIdentifier("typed")
                );
                
              return result;
            }
            
          if (HAS_OUT(chunk->channels))
            return new CASTIdentifier(chunk->name);
         
          int bytesToShift = chunk->serverOffset%4;
          CASTExpression *target = buildRegisterVar(chunk->serverIndex);
          if (bytesToShift)
            target = new CASTBinaryOp(">>",
              target,
              new CASTIntegerConstant(bytesToShift*8)
            );
          if (chunk->size<4)
            target = new CASTBinaryOp("&", 
              new CASTBrackets(
                target),
              new CASTHexConstant(~(~(1<<(chunk->size*8))+1))
            );
          return chunk->type->buildTypeCast(0, target);
        }    
    }    

  chunk = findChunk(registers[channel], chunkID);
  if (chunk)
    {
      return chunk->getCachedOutput(0);
    }

  panic("Cannot build FCC data target expression (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}      
