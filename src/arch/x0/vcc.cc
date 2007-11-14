#include "cast.h"
#include "arch/x0.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

// BUFFER ALLOCATION: 
//    1 for every IN string plus
//    the maximal number of strings _or_ varbufs used in an OUT or EXC channel
//
// COPYING:
//    IN: pointer to the element in the message is passed
//    INOUT: element is copied to a buffer; pointer to that buffer is passed
//    OUT: pointer to an empty buffer is passed
//
// EXCHANGING THE BUFFER:
//    always possible for INOUTs, but input buffer is discarded automatically
//
// ASSUMING OWNERSHIP:
//    *** can the macro check whether the string is still in the message?
//    *** may have to pass in a pointer to the message buffer? but where
//    *** to get that?

int calculateStringPriority(int channels)

{
  int prio = PRIO_IN_STRING;
        
  if (HAS_OUT(channels)) 
    prio = PRIO_OUT_STRING;
  if (HAS_IN_AND_OUT(channels))
    prio = PRIO_INOUT_STRING;  
    
  return prio;
}  

ChunkID CMSConnectionX::getVariableCopyChunk(int channels, int minSize, int typSize, int maxSize, const char *name, CBEType *type)

{
  CBEType *mwType = msFactory->getMachineWordType();
  bool shouldBeString = isWorthString(minSize, typSize, maxSize);
  int thisChunk = ++chunkID;

  dprintln("getVCC channels=%d size=[%d,%d,%d] name=%s -> chunkID %d", channels, minSize, typSize, maxSize, name, thisChunk);
  
  for (int i=0;i<numChannels;i++) 
    if (channels&(1<<i))
      {
        if (shouldBeString) 
          {
            strings[i]->addWithPrio(new CMSChunkX(thisChunk, name, SIZEOF_STRINGDOPE, maxSize, CHUNK_NOALIGN, new CBEOpaqueType("idl4_strdope_t", 8, 4), type, channels, CHUNK_STANDARD), calculateStringPriority(channels));
            dprintln("  (string)");
          } else {
                   if (minSize<maxSize)
                     {
                       int allocatedSize = ((maxSize+3)/4)*4;
                       const char *sizeName = mprintf("_%s_size", name);
                       const char *bufferName = mprintf("_%s_buf", name);
                       vars[i]->addWithPrio(new CMSChunkX(thisChunk, name, allocatedSize, allocatedSize, CHUNK_NOALIGN, type, type, channels, CHUNK_ALLOC_CLIENT|CHUNK_ALLOC_SERVER|CHUNK_XFER_COPY), PRIO_VARIABLE);
                       dwords[i]->addWithPrio(new CMSChunkX(thisChunk, sizeName, mwType->getFlatSize(), mwType->getFlatSize(), CHUNK_ALIGN4, mwType, NULL, channels, CHUNK_STANDARD), calculatePriority(channels));
                       if (HAS_OUT(channels) && (i==CHANNEL_OUT))
                         strings[i]->addWithPrio(new CMSChunkX(thisChunk, bufferName, SIZEOF_STRINGDOPE, 0, CHUNK_ALIGN4, new CBEOpaqueType("idl4_strdope_t", 8, 4), type, CHANNEL_OUT, CHUNK_ALLOC_SERVER), PRIO_TEMPORARY);
                       dprintln("  (variable)");
                     } else {
                       	      int align = 1;
                              while (align<maxSize) 
                                align*=2;
                                
                              dwords[i]->addWithPrio(new CMSChunkX(thisChunk, name, maxSize, maxSize, align, type, NULL, channels, CHUNK_STANDARD), calculatePriority(channels));
                              dprintln("  (fixed)");
                            }
                 }    
      }
    
  return thisChunk;  
}

CASTStatement *CMSConnectionX::assignVCCSizeAndDataToBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue, CASTExpression *size)

{
  CMSChunkX *chunk;

  /* If this is a variable-sized chunk (and not an indirect string), we
     need to copy the value to the buffer and update its size */

  chunk = findChunk(vars[channel], chunkID);
  if (chunk)
    {
      CASTStatement *result = NULL;
      
      addTo(result, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("idl4_memcpy"),
          knitExprList(new CASTUnaryOp("&",
                         new CASTIndexOp(
                           new CASTBinaryOp(".",
                             buildSourceBufferRvalue(channel),
                             new CASTIdentifier("_varbuf")),
                           new CASTIdentifier("_varidx"))),
                       rvalue,
                       buildSizeDwordAlign(size, chunk->type->getFlatSize(), false))))
      );
      
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("+=",
          new CASTIdentifier("_varidx"),
          buildSizeDwordAlign(size, chunk->type->getFlatSize(), true)))
      );    
      
      addTo(result, assignFCCDataToBuffer(channel, chunkID, size));
      
      return result;
    }

  /* If the chunk is large, it might be an indirect string */
  
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
            new CASTIdentifier("snd_addr")),
          new CASTCastOp(
            new CASTDeclaration(
              new CASTTypeSpecifier(
                new CASTIdentifier("void")),
              new CASTDeclarator((CASTIdentifier*)NULL, new CASTPointer(1))),
            rvalue, CASTStandardCast)))
      );

      CASTExpression *sizeExpr = size;
      if (chunk->contentType->getFlatSize()>1)
        sizeExpr = new CASTBinaryOp("*",
          sizeExpr,
          new CASTFunctionOp(
            new CASTIdentifier("sizeof"),
            new CASTDeclarationExpression(
              chunk->contentType->buildDeclaration(NULL)))
        );
      
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            new CASTBinaryOp(".",
              buildSourceBufferRvalue(channel),
              new CASTIdentifier(chunk->name)),
            new CASTIdentifier("snd_size")),
          sizeExpr))
      );
          
      return result;
    }

  panic("Cannot assign VCC data to buffer (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTStatement *CMSConnectionX::provideVCCTargetBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue, CASTExpression *size)

{
  CMSChunkX *chunk;

  chunk = findChunk(vars[channel], chunkID);
  if (chunk)
    { 
      chunk->targetBuffer = rvalue;
      chunk->targetBufferSize = size;
      return NULL;
    }
    
  chunk = findChunk(strings[channel], chunkID);
  if (chunk)
    { 
      chunk->targetBuffer = rvalue;
      chunk->targetBufferSize = size;
      return NULL;
    }
    
  panic("Cannot provide VCC target buffer (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTStatement *CMSConnectionX::assignVCCDataFromBuffer(int channel, ChunkID chunkID, CASTExpression *lvalue)

{
  CMSChunkX *chunk;

  /* If this is a variable-sized chunk (and not an indirect string), we
     need to copy the value to the buffer and update its size */

  chunk = findChunk(vars[channel], chunkID);
  if (chunk)
    {
      CASTStatement *result = NULL;
      
      /* We may only use the value in the buffer directly if 
           a) we are on the server (CHANNEL_IN) and
           b) the value will not be modified (i.e. is not inout) */
  
      if ((channel!=CHANNEL_IN) || (HAS_OUT(chunk->channels)))
        {
          CMSChunkX *strChunk = findChunk(strings[CHANNEL_OUT], chunkID);
          
          if (!strChunk)
            panic("No temporary buffer allocated for chunk %d", chunkID);
            
          if (channel==CHANNEL_IN)
            addTo(result, new CASTExpressionStatement(
              new CASTBinaryOp("=",
                lvalue->clone(), 
                chunk->contentType->buildTypeCast(1,
                  new CASTBinaryOp(".",
                    new CASTBinaryOp(".",
                      buildSourceBufferRvalue(CHANNEL_OUT),
                      new CASTIdentifier(strChunk->name)),
                    new CASTIdentifier("rcv_addr")))))
            );    
            
          addTo(result, new CASTExpressionStatement(
            new CASTFunctionOp(
              new CASTIdentifier("idl4_memcpy"),
              knitExprList(lvalue,
                           new CASTUnaryOp("&",
                            new CASTIndexOp(
                              new CASTBinaryOp(".",
                                buildTargetBufferRvalue(channel),
                                new CASTIdentifier("_varbuf")),
                              new CASTIdentifier("_varidx"))),
                           buildSizeDwordAlign(
                             buildFCCDataTargetExpr(channel, chunkID), 
                             chunk->type->getFlatSize(), false))))
          );
        } else addTo(result, new CASTExpressionStatement(
                 new CASTBinaryOp("=",
                   lvalue,
                   chunk->contentType->buildTypeCast(1,
                     new CASTUnaryOp("&",
                       new CASTIndexOp(
                         new CASTBinaryOp(".",
                           buildTargetBufferRvalue(channel),
                           new CASTIdentifier("_varbuf")),
                         new CASTIdentifier("_varidx"))))))
               );               
      
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("+=",
          new CASTIdentifier("_varidx"),
          buildSizeDwordAlign(buildFCCDataTargetExpr(channel, chunkID), chunk->type->getFlatSize(), true)))
      );    
      
      return result;
    }

  /* If the chunk is large, it might be an indirect string */
  
  chunk = findChunk(strings[channel], chunkID);
  if (chunk)
    {
      CASTStatement *result = NULL;
      
      if (channel!=CHANNEL_IN)
        {
          addTo(result, new CASTExpressionStatement(
            new CASTFunctionOp(
              new CASTIdentifier("idl4_memcpy"),
              knitExprList(lvalue,
                           new CASTBinaryOp(".",
                             new CASTBinaryOp(".",
                               buildTargetBufferRvalue(channel),
                               new CASTIdentifier(chunk->name)),
                             new CASTIdentifier("rcv_addr")),
                           new CASTBinaryOp(">>", 
                             new CASTBrackets(
                               new CASTBinaryOp("+",
                                 new CASTBinaryOp(".",
                                   new CASTBinaryOp(".",
                                     buildTargetBufferRvalue(channel),
                                     new CASTIdentifier(chunk->name)),
                                   new CASTIdentifier("snd_size")),
                                 new CASTIntegerConstant(3))),
                               new CASTIntegerConstant(2)))))
          );
        } else {
                 /* Note: Fiasco does not set the snd_addr; for compatibility,
                    we use rcv_addr */
        
                 addTo(result, new CASTExpressionStatement(
                   new CASTBinaryOp("=",
                     lvalue,
                     chunk->contentType->buildTypeCast(1, 
                       new CASTBinaryOp(".",
                         new CASTBinaryOp(".",
                           buildTargetBufferRvalue(channel),
                           new CASTIdentifier(chunk->name)),
                         new CASTIdentifier("rcv_addr")))))
                 );
               }  
          
      return result;
    }

  panic("Cannot assign VCC data from buffer (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTStatement *CMSConnectionX::assignVCCSizeFromBuffer(int channel, ChunkID chunkID, CASTExpression *lvalue)

{
  CMSChunkX *chunk;

  chunk = findChunk(vars[channel], chunkID);
  if (chunk)
    return assignFCCDataFromBuffer(channel, chunkID, lvalue);

  chunk = findChunk(strings[channel], chunkID);
  if (chunk)
    {
      CASTStatement *result = NULL;

      CASTExpression *sizeExpr = new CASTBinaryOp(".",
        new CASTBinaryOp(".",
          buildTargetBufferRvalue(channel),
          new CASTIdentifier(chunk->name)),
        new CASTIdentifier("snd_size")
      );
      
      if (chunk->contentType->getFlatSize()>1)
        sizeExpr = new CASTBinaryOp("/",
          sizeExpr,
          new CASTFunctionOp(
            new CASTIdentifier("sizeof"),
            new CASTDeclarationExpression(
              chunk->contentType->buildDeclaration(NULL)))
        );
      
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          lvalue,
          sizeExpr))
      );
          
      return result;
    }

  panic("Cannot assign VCC size from buffer (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTStatement *CMSConnectionX::buildVCCServerPrealloc(int channel, ChunkID chunkID, CASTExpression *lvalue)

{
  CMSChunkX *chunk;

  chunk = findChunk(vars[channel], chunkID);
  if (chunk)
    {
      if ((channel!=CHANNEL_IN) && (!HAS_IN(chunk->channels)))
        {
          CMSChunkX *strChunk = findChunk(strings[channel], chunkID);
          assert(strChunk);
          
          return new CASTExpressionStatement(
            new CASTBinaryOp("=",
              lvalue->clone(), 
                chunk->contentType->buildTypeCast(1, 
                new CASTBinaryOp(".",
                  new CASTBinaryOp(".",
                    buildSourceBufferRvalue(channel),
                    new CASTIdentifier(strChunk->name)),
                  new CASTIdentifier("rcv_addr"))))
          );    
        }
    }    
    
  /* Strings have their own buffer and do not need to be preallocated one */
   
  chunk = findChunk(strings[channel], chunkID);
  if (chunk)
    {
      if ((channel!=CHANNEL_IN) && (!HAS_IN(chunk->channels)))
        {
          return new CASTExpressionStatement(
            new CASTBinaryOp("=",
              lvalue->clone(), 
                chunk->contentType->buildTypeCast(1, 
                new CASTBinaryOp(".",
                  new CASTBinaryOp(".",
                    buildSourceBufferRvalue(channel),
                    new CASTIdentifier(chunk->name)),
                  new CASTIdentifier("rcv_addr"))))
          );    
        }
        
      return NULL;
    }    
  
  panic("Cannot build VCC prealloc (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}
