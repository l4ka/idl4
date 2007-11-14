#include "arch/v4.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

ChunkID CMSConnection4::getFixedCopyChunk(int channels, int size, const char *name, CBEType *type)

{
  int thisChunk = ++chunkID;

  dprintln("getFCC channels=%d size=%d name=%s -> chunkID %d", channels, size, name, thisChunk);

  for (int i=0;i<numChannels;i++)
    if (channels&(1<<i))
      {
        if (!(channelInitialized & CHANNEL_MASK(i)))
          {
            channelInitialized |= CHANNEL_MASK(i);
            continue;
          }
          
        if (size<400)
          mem_fixed[i]->addWithPrio(new CMSChunk4(thisChunk, size, name, type, type, channels, type->getAlignment(), CHUNK_XFER_COPY), alignmentPrio(type->getAlignment()));
          else strings[i]->add(new CMSChunk4(thisChunk, size, name, type, type, channels, type->getAlignment(), CHUNK_XFER_COPY));
      }
    
  return thisChunk;  
}

CASTStatement *CMSConnection4::assignFCCDataToBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue)

{
  CMSChunk4 *chunk;

  chunk = findChunk(reg_fixed[channel], chunkID);
  if (chunk)
    return chunk->type->buildAssignment(
      new CASTBinaryOp(".",
        buildSourceBufferRvalue(channel),
        new CASTIdentifier(chunk->name)),
      rvalue
    );

  chunk = findChunk(mem_fixed[channel], chunkID);
  if (chunk)
    return chunk->type->buildAssignment(
      new CASTBinaryOp(".",
        buildMemmsgRvalue(channel, (channel!=CHANNEL_IN)),
        new CASTIdentifier(chunk->name)),
        rvalue
    );

  chunk = findChunk(special[channel], chunkID);
  if (chunk)
    {
      assert(!chunk->cachedInExpr);
      assert(chunk->specialPos == POS_PRIVILEGES);
      chunk->cachedInExpr = rvalue;
      return NULL;
    }

  chunk = findChunk(strings[channel], chunkID);
  if (chunk)
    {
      CASTStatement *result = NULL;
      
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            new CASTBinaryOp(".",
              buildSourceBufferRvalue(channel),
              new CASTIdentifier(chunk->name)),
            new CASTIdentifier("len")),
          new CASTBinaryOp("<<",
            new CASTIntegerConstant(chunk->size),
            new CASTIntegerConstant(10))))
      );
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            new CASTBinaryOp(".",
              buildSourceBufferRvalue(channel),
              new CASTIdentifier(chunk->name)),
            new CASTIdentifier("ptr")),
          new CASTUnaryOp("(void*)&",
            rvalue)))
      );
      
      return result;
    }

  panic("Cannot assign FCC data to buffer (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTStatement *CMSConnection4::assignFCCDataFromBuffer(int channel, ChunkID chunkID, CASTExpression *lvalue)

{
  CMSChunk4 *chunk;

  if (chunkID==0)
    return new CASTExpressionStatement(
      new CASTBinaryOp("=",
        lvalue,
        buildLabelExpr())
    );

  /* Dword values can be assigned directly */

  chunk = findChunk(reg_fixed[channel], chunkID);
  if (chunk)
    {
      return chunk->type->buildAssignment(
        lvalue, 
        new CASTBinaryOp(".",
          buildTargetBufferRvalue(channel),
          new CASTIdentifier(chunk->name))
      );
    }

  chunk = findChunk(mem_fixed[channel], chunkID);
  if (chunk)
    {
      return chunk->type->buildAssignment(
        lvalue, 
        new CASTBinaryOp(".",
          buildMemmsgRvalue(channel, (channel==CHANNEL_IN)),
          new CASTIdentifier(chunk->name))
      );
    }

  chunk = findChunk(strings[channel], chunkID);
  if (chunk)
    return assignStringFromBuffer(channel, chunk, new CASTUnaryOp("&", lvalue));

  panic("Cannot assign FCC data from buffer (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTExpression *CMSConnection4::buildFCCDataTargetExpr(int channel, ChunkID chunkID)

{
  CMSChunk4 *chunk;

  chunk = findChunk(reg_fixed[channel], chunkID);
  if (chunk)
    {
      return new CASTBinaryOp(".",
        buildTargetBufferRvalue(channel),
        new CASTIdentifier(chunk->name)
      );    
    }
    
  chunk = findChunk(special[channel], chunkID);
  if (chunk)
    {
      assert(chunk->specialPos == POS_PRIVILEGES);
      return new CASTBinaryOp("&",
        new CASTBrackets(
          new CASTBinaryOp(">>",
            new CASTBinaryOp(".",
              buildTargetBufferRvalue(channel),
              new CASTIdentifier("_msgtag")),
            new CASTIntegerConstant(16))),
        new CASTIntegerConstant(7)
      );    
    }

  chunk = findChunk(mem_fixed[channel], chunkID);
  if (chunk)
    {
      return new CASTBinaryOp(".",
        new CASTBinaryOp("->",
          new CASTIdentifier("_memmsg"),
          buildChannelIdentifier(channel)),
        new CASTIdentifier(chunk->name)
      );    
    }

  chunk = findChunk(strings[channel], chunkID);
  if (chunk)
    {
      return new CASTUnaryOp("*",
        chunk->contentType->buildTypeCast(1, 
          new CASTBinaryOp(".",
            new CASTBinaryOp(".",
              new CASTBinaryOp("->",
                new CASTIdentifier("_par"),
                new CASTIdentifier("_buf")),
              new CASTIndexOp(
                new CASTIdentifier("_str"),
                new CASTIntegerConstant(15 - chunk->bufferIndex))),
              new CASTIdentifier("ptr")))
      );
    }

  panic("Cannot build FCC data target expression (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTExpression *CMSConnection4::buildFCCDataSourceExpr(int channel, ChunkID chunkID)

{
  CMSChunk4 *chunk;

  chunk = findChunk(reg_fixed[channel], chunkID);
  if (chunk)
    {
      return new CASTBinaryOp(".",
        new CASTBinaryOp("->",
          new CASTIdentifier("_par"), 
          buildChannelIdentifier(channel)),
        new CASTIdentifier(chunk->name)
      );    
    }

  chunk = findChunk(mem_fixed[channel], chunkID);
  if (chunk)
    {
      return new CASTBinaryOp(".",
        new CASTBinaryOp("->",
          new CASTIdentifier("_memmsg"), 
          buildChannelIdentifier(channel)),
        new CASTIdentifier(chunk->name)
      );    
    }

  chunk = findChunk(strings[channel], chunkID);
  if (chunk)
    {
      return new CASTUnaryOp("*",
        chunk->contentType->buildTypeCast(1, 
          new CASTBinaryOp(".",
            new CASTBinaryOp(".",
              new CASTBinaryOp("->",
                new CASTIdentifier("_par"),
                new CASTIdentifier("_buf")),
              new CASTIndexOp(
                new CASTIdentifier("_str"),
                new CASTIntegerConstant(15 - chunk->bufferIndex))),
              new CASTIdentifier("ptr")))
      );    
    }
    
  panic("Cannot build FCC source expression (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

