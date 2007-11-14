#include "arch/x0.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

ChunkID CMSConnectionX::getMapChunk(int channels, int size, const char *name)

{
  int thisChunk = ++chunkID;
  
  assert(size==1); /* a pretty conservative assumption... 
                      Hazelnut supports only one anyway */
  
  for (int i=0;i<numChannels;i++)
    if (channels&(1<<i))
      dwords[i]->addWithPrio(new CMSChunkX(thisChunk, name, size * 8, size * 8, CHUNK_ALIGN8, new CBEOpaqueType("idl4_fpage_t", 8, 4), NULL, channels, CHUNK_STANDARD), PRIO_MAP);
    
  return thisChunk;  
}

CASTStatement *CMSConnectionX::assignFMCFpageToBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue)

{
  CMSChunkX *chunk;

  chunk = findChunk(registers[channel], chunkID);
  if (chunk)
    {
      chunk->setCachedInput(0, new CASTBinaryOp(".",
        rvalue,
        new CASTIdentifier("base"))
      );
      chunk->setCachedInput(1, new CASTBinaryOp(".",
        rvalue,
        new CASTIdentifier("fpage"))
      );
      return NULL;
    }

  chunk = findChunk(dwords[channel], chunkID);
  if (chunk)
    {
      CASTStatement *result = NULL;
      
      CASTExpression *target = new CASTBinaryOp(".",
        buildSourceBufferRvalue(channel),
        new CASTIdentifier(chunk->name)
      );
      
      if (!rvalue->equals(target))
        addTo(result, new CASTExpressionStatement(
          new CASTBinaryOp("=", target, rvalue))
        );    

      return result;
    }

  panic("Cannot assign FMC fpage and base to buffer (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTStatement *CMSConnectionX::assignFMCFpageFromBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue)

{
  CMSChunkX *chunk;

  chunk = findChunk(registers[channel], chunkID);
  if (chunk)
    {
      chunk->setCachedOutput(0, new CASTBinaryOp(".",
        rvalue,
        new CASTIdentifier("base"))
      );
      chunk->setCachedOutput(1, new CASTBinaryOp(".",
        rvalue,
        new CASTIdentifier("fpage"))
      );
      
      return NULL;    
    }

  chunk = findChunk(dwords[channel], chunkID);
  if (chunk)
    {
      return new CASTExpressionStatement(
        new CASTBinaryOp("=",
          rvalue, 
          new CASTBinaryOp(".",
            buildTargetBufferRvalue(channel),
            new CASTIdentifier(chunk->name)))
      );
    }

  panic("Cannot assign FMC fpage from buffer (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTExpression *CMSConnectionX::buildFMCFpageSourceExpr(int channel, ChunkID chunkID)

{
  CMSChunkX *chunk;

  dprintln("building fmc fpage source expr for channel %d", channel);

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
        
  chunk = findChunk(dwords[channel], chunkID);
  if (chunk)
    {
      return new CASTBinaryOp(".",
        buildSourceBufferRvalue(channel),
        new CASTIdentifier(chunk->name)
      );
    }

  panic("Cannot build FMC fpage source expression (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTExpression *CMSConnectionX::buildFMCFpageTargetExpr(int channel, ChunkID chunkID)

{
  CMSChunkX *chunk;

  dprintln("building fmc fpage target expr for channel %d", channel);

  chunk = findChunk(registers[channel], chunkID);
  if (chunk)
    {
      return new CASTCastOp(
        new CASTDeclaration(
          NULL, 
          new CASTDeclarator(
            new CASTIdentifier("idl4_fpage_t"))),
        new CASTArrayInitializer(
          knitExprList(
            new CASTLabeledExpression(
              new CASTIdentifier("base"),
              buildRegisterVar(chunk->serverIndex)),
            new CASTLabeledExpression(
              new CASTIdentifier("fpage"),
              buildRegisterVar(chunk->serverIndex + 1)))),
        CASTStandardCast
      );
    }    
        
  chunk = findChunk(dwords[channel], chunkID);
  if (chunk)
    {
      return new CASTBinaryOp(".",
        buildTargetBufferRvalue(channel),
        new CASTIdentifier(chunk->name)
      );
    }

  panic("Cannot build FMC fpage target expression (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}
