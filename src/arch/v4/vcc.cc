#include "arch/v4.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

ChunkID CMSConnection4::getVariableCopyChunk(int channels, int minSize, int typSize, int maxSize, const char *name, CBEType *type)

{
  CBEType *mwType = msFactory->getMachineWordType();
  int thisChunk = ++chunkID;

  dprintln("getVCC channels=%d size=[%d,%d,%d] name=%s -> chunkID %d", channels, minSize, typSize, maxSize, name, thisChunk);

  for (int i=0;i<numChannels;i++)
    if (channels&(1<<i))
      {
        if (maxSize!=UNKNOWN)
          {
            const char *sizeName = mprintf("_%s_size", name);
            const char *bufferName = mprintf("_%s_buf", name);
            mem_variable[i]->add(new CMSChunk4(thisChunk, maxSize, bufferName, type, type, channels, 1, CHUNK_XFER_COPY));
            mem_fixed[i]->add(new CMSChunk4(thisChunk, mwType->getFlatSize(), sizeName, mwType, mwType, channels, mwType->getAlignment(), CHUNK_XFER_COPY));
            strings[i]->add(new CMSChunk4(thisChunk, maxSize, name, NULL, type, channels, 1, 0));
          } else strings[i]->add(new CMSChunk4(thisChunk, UNKNOWN, name, type, type, channels, 1, CHUNK_XFER_COPY));
      }  
    
  return thisChunk;  
}

CASTStatement *CMSConnection4::assignVCCSizeAndDataToBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue, CASTExpression *size)

{
  CMSChunk4 *chunk;

  chunk = findChunk(reg_variable[channel], chunkID);
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
                           new CASTIdentifier("_regvaridx"))),
                       rvalue,
                       buildSizeDwordAlign(size, chunk->type->getFlatSize(), false))))
      );
      
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("+=",
          new CASTIdentifier("_regvaridx"),
          buildSizeDwordAlign(size, chunk->type->getFlatSize(), true)))
      );    
      
      addTo(result, assignFCCDataToBuffer(channel, chunkID, size));
      
      return result;
    }

  chunk = findChunk(mem_variable[channel], chunkID);
  if (chunk)
    {
      CASTStatement *result = NULL;
      
      addTo(result, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("idl4_memcpy"),
          knitExprList(new CASTUnaryOp("&",
                         new CASTIndexOp(
                           new CASTBinaryOp(".",
                             buildMemmsgRvalue(channel, (channel!=CHANNEL_IN)),
                             new CASTIdentifier("_varbuf")),
                           new CASTIdentifier("_memvaridx"))),
                       rvalue,
                       buildSizeDwordAlign(size, chunk->type->getFlatSize(), false))))
      );
      
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("+=",
          new CASTIdentifier("_memvaridx"),
          buildSizeDwordAlign(size, chunk->type->getFlatSize(), true)))
      );    
      
      addTo(result, assignFCCDataToBuffer(channel, chunkID, size));
      
      return result;
    }

  chunk = findChunk(strings[channel], chunkID);
  if (chunk)
    {
      CASTStatement *result = NULL;

      CASTExpression *sizeExpr = size;
      if (chunk->contentType->getFlatSize()>1)
        sizeExpr = new CASTBrackets(
          new CASTBinaryOp("*",
            sizeExpr,
            new CASTFunctionOp(
              new CASTIdentifier("sizeof"),
              new CASTDeclarationExpression(
                chunk->contentType->buildDeclaration(NULL))))
        );
      
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            new CASTBinaryOp(".",
              buildSourceBufferRvalue(channel),
              new CASTIdentifier(chunk->name)),
              new CASTIdentifier("len")),
          new CASTBinaryOp("<<",
            sizeExpr,
            new CASTIntegerConstant(10))))
       );

      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            new CASTBinaryOp(".",
              buildSourceBufferRvalue(channel),
              new CASTIdentifier(chunk->name)),
            new CASTIdentifier("ptr")),
          new CASTUnaryOp("(void*)",
            rvalue)))
       );
      
      return result;
    }  

  panic("Cannot assign VCC data to buffer (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTStatement *CMSConnection4::assignStringFromBuffer(int channel, CMSChunk4 *chunk, CASTExpression *lvalue)

{
  CASTStatement *result = NULL;

  if (channel != CHANNEL_IN)
    {
      addTo(result, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("L4_StoreBR"),
          knitExprList(
            new CASTIntegerConstant(2*(chunk->offset+1)),
            new CASTUnaryOp("(L4_Word_t*)",
              new CASTUnaryOp("&",
                new CASTBinaryOp(".",
                  new CASTBinaryOp(".",
                    buildTargetBufferRvalue(channel),
                    new CASTIdentifier(chunk->name)),
                  new CASTIdentifier("ptr")))))))
      );
              
      addTo(result, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("idl4_memcpy"),
          knitExprList(lvalue,
                       new CASTBinaryOp(".",
                         new CASTBinaryOp(".",
                           buildTargetBufferRvalue(channel),
                           new CASTIdentifier(chunk->name)),
                         new CASTIdentifier("ptr")),
                       new CASTBinaryOp(">>", 
                         new CASTBrackets(
                           new CASTBinaryOp("+",
                             new CASTBinaryOp(".",
                               new CASTBinaryOp(".",
                                 buildTargetBufferRvalue(channel),
                                 new CASTIdentifier(chunk->name)),
                               new CASTIdentifier("len")),
                             new CASTHexConstant(0xC00))),
                           new CASTIntegerConstant(12)))))
      );
    } else {
             addTo(result, new CASTExpressionStatement(
               new CASTBinaryOp("=",
                 lvalue,
                 chunk->contentType->buildTypeCast(1, 
                   new CASTBinaryOp(".",
                     new CASTBinaryOp(".",
                       new CASTBinaryOp("->",
                         new CASTIdentifier("_par"),
                         new CASTIdentifier("_buf")),
                       new CASTIndexOp(
                         new CASTIdentifier("_str"),
                         new CASTIntegerConstant(15 - chunk->bufferIndex))),
                     new CASTIdentifier("ptr")))))
             );
           }
      
  return result;
}

CASTStatement *CMSConnection4::assignVCCDataFromBuffer(int channel, ChunkID chunkID, CASTExpression *lvalue)

{
  CMSChunk4 *chunk;

  chunk = findChunk(reg_variable[channel], chunkID);
  if (chunk)
    {
      CASTStatement *result = NULL;
      
      if ((channel!=CHANNEL_IN) || (HAS_OUT(chunk->channels)))
        {
          if (channel==CHANNEL_IN)
            {
              CMSChunk4 *strChunk = findChunk(strings[channel], chunkID);
              assert(strChunk && (strChunk->bufferIndex != UNKNOWN));
          
              addTo(result, new CASTExpressionStatement(
                new CASTBinaryOp("=",
                  lvalue,
                  chunk->contentType->buildTypeCast(1, 
                    new CASTBinaryOp(".",
                      new CASTBinaryOp(".",
                        new CASTBinaryOp("->",
                          new CASTIdentifier("_par"),
                          new CASTIdentifier("_buf")),
                        new CASTIndexOp(
                          new CASTIdentifier("_str"),
                          new CASTIntegerConstant(15 - strChunk->bufferIndex))),
                      new CASTIdentifier("ptr"))))
                   )
              );
            }

          addTo(result, new CASTExpressionStatement(
            new CASTFunctionOp(
              new CASTIdentifier("idl4_memcpy"),
              knitExprList(
                lvalue->clone(),
                new CASTUnaryOp("&",
                  new CASTIndexOp(
                    new CASTBinaryOp(".",
                      buildTargetBufferRvalue(channel),
                      new CASTIdentifier("_varbuf")),
                    new CASTIdentifier("_regvaridx"))),
                buildSizeDwordAlign(
                  buildFCCDataTargetExpr(channel, chunkID), 
                  chunk->type->getFlatSize(), false))))
          );
        } else {
                 addTo(result, new CASTExpressionStatement(
                   new CASTBinaryOp("=",
                     lvalue,
                     chunk->type->buildTypeCast(1, 
                       new CASTUnaryOp("&",
                         new CASTIndexOp(
                           new CASTBinaryOp(".",
                             buildTargetBufferRvalue(channel),
                             new CASTIdentifier("_varbuf")),
                           new CASTIdentifier("_regvaridx"))))))
                 );
               }

      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("+=",
          new CASTIdentifier("_regvaridx"),
          buildSizeDwordAlign(buildFCCDataTargetExpr(channel, chunkID), chunk->type->getFlatSize(), true)))
      );    
      
      return result;
    }  

  chunk = findChunk(mem_variable[channel], chunkID);
  if (chunk)
    {
      CASTStatement *result = NULL;
      
      if ((channel!=CHANNEL_IN) || (HAS_OUT(chunk->channels)))
        {
          if (channel==CHANNEL_IN)
            {
              CMSChunk4 *strChunk = findChunk(strings[channel], chunkID);
              assert(strChunk && (strChunk->bufferIndex != UNKNOWN));
          
              addTo(result, new CASTExpressionStatement(
                new CASTBinaryOp("=",
                  lvalue,
                  chunk->contentType->buildTypeCast(1, 
                    new CASTBinaryOp(".",
                      new CASTBinaryOp(".",
                        new CASTBinaryOp("->",
                          new CASTIdentifier("_par"),
                          new CASTIdentifier("_buf")),
                        new CASTIndexOp(
                          new CASTIdentifier("_str"),
                          new CASTIntegerConstant(15 - strChunk->bufferIndex))),
                      new CASTIdentifier("ptr"))))
                   )
              );
            }

          addTo(result, new CASTExpressionStatement(
            new CASTFunctionOp(
              new CASTIdentifier("idl4_memcpy"),
              knitExprList(
                lvalue->clone(),
                new CASTUnaryOp("&",
                  new CASTIndexOp(
                    new CASTBinaryOp(".",
                      buildMemmsgRvalue(channel, (channel==CHANNEL_IN)),
                      new CASTIdentifier("_varbuf")),
                    new CASTIdentifier("_memvaridx"))),
                buildSizeDwordAlign(
                  buildFCCDataTargetExpr(channel, chunkID), 
                  chunk->type->getFlatSize(), false))))
          );
        } else {
                 addTo(result, new CASTExpressionStatement(
                   new CASTBinaryOp("=",
                     lvalue,
                     chunk->type->buildTypeCast(1, 
                       new CASTUnaryOp("&",
                         new CASTIndexOp(
                           new CASTBinaryOp(".",
                             buildMemmsgRvalue(channel, (channel==CHANNEL_IN)),
                             new CASTIdentifier("_varbuf")),
                           new CASTIdentifier("_memvaridx"))))))
                 );
               }

      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("+=",
          new CASTIdentifier("_memvaridx"),
          buildSizeDwordAlign(buildFCCDataTargetExpr(channel, chunkID), chunk->type->getFlatSize(), true)))
      );    
      
      return result;
    }  

  chunk = findChunk(strings[channel], chunkID);
  if (chunk)
    return assignStringFromBuffer(channel, chunk, lvalue);
      
  panic("Cannot assign VCC data from buffer (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTStatement *CMSConnection4::assignVCCSizeFromBuffer(int channel, ChunkID chunkID, CASTExpression *lvalue)

{
  CMSChunk4 *chunk;

  chunk = findChunk(reg_variable[channel], chunkID);
  if (chunk)
    return assignFCCDataFromBuffer(channel, chunkID, lvalue);

  chunk = findChunk(mem_variable[channel], chunkID);
  if (chunk)
    return assignFCCDataFromBuffer(channel, chunkID, lvalue);

  chunk = findChunk(strings[channel], chunkID);
  if (chunk)
    {
      CASTStatement *result = NULL;

      CASTExpression *sizeExpr = new CASTBinaryOp(">>",
        new CASTBinaryOp(".",
          new CASTBinaryOp(".",
            buildTargetBufferRvalue(channel),
            new CASTIdentifier(chunk->name)),
          new CASTIdentifier("len")),
        new CASTIntegerConstant(10)
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

CASTStatement *CMSConnection4::buildVCCServerPrealloc(int channel, ChunkID chunkID, CASTExpression *lvalue)

{
  CMSChunk4 *chunk;

  chunk = findChunk(reg_variable[channel], chunkID);
  if (chunk)
    {
      CMSChunk4 *strChunk = findChunk(strings[channel], chunkID);

      assert(channel!=CHANNEL_IN);
      assert(strChunk);
          
      return new CASTExpressionStatement(
        new CASTBinaryOp("=",
          lvalue->clone(), 
          chunk->contentType->buildTypeCast(1, 
            new CASTBinaryOp(".",
              new CASTBinaryOp(".",
                new CASTBinaryOp("->",
                  new CASTIdentifier("_par"),
                  new CASTIdentifier("_buf")),
                new CASTIndexOp(
                  new CASTIdentifier("_str"),
                  new CASTIntegerConstant(15 - strChunk->bufferIndex))),
              new CASTIdentifier("ptr"))))
          );
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
                    new CASTBinaryOp("->",
                      new CASTIdentifier("_par"),
                      new CASTIdentifier("_buf")),
                    new CASTIndexOp(
                      new CASTIdentifier("_str"),
                      new CASTIntegerConstant(15 - chunk->bufferIndex))),
                  new CASTIdentifier("ptr"))))
          );
        }
        
      return NULL;
    }    
  
    
  panic("Cannot build VCC prealloc (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTStatement *CMSConnection4::provideVCCTargetBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue, CASTExpression *size)

{
  CMSChunk4 *chunk;

  chunk = findChunk(strings[channel], chunkID);
  if (chunk)
    { 
      chunk->targetBuffer = rvalue;
      chunk->targetBufferSize = size;
      return NULL;
    }
    
/*  chunk = findChunk(vars[channel], chunkID);
  if (chunk)
    { 
      chunk->targetBuffer = rvalue;
      chunk->targetBufferSize = size;
      return NULL;
    } */
    
  panic("Cannot provide VCC target buffer (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}
