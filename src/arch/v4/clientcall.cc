#include "arch/v4.h"

int CMSConnection4::getMaxUntypedWordsInChannel(int channel)

{
  /* Find out how many bytes are allocated in MRs */

  int numBytes = globals.word_size;
  forAllMS(reg_fixed[channel],
    {
      CMSChunk4 *chunk = (CMSChunk4*)item;
      int endsAt = chunk->offset+chunk->size;
      if (numBytes<endsAt) numBytes = endsAt;
    }  
  );

  return (numBytes-1)/globals.word_size;
}

CASTExpression *CMSConnection4::buildMsgTag(int channel)

{
  /* Find out how many typed elements we have. Note that if a memory message
     is to be used, we need an additional StringItem. */

  int typedMRs = 0;
  forAllMS(strings[channel], 
    if (((CMSChunk4*)item)->flags & CHUNK_XFER_COPY)
      typedMRs+=2
  );
  forAllMS(fpages[channel], typedMRs+=2);
  if (!mem_fixed[channel]->isEmpty() || !mem_variable[channel]->isEmpty())
    typedMRs += 2;

  /* Build the ID part of the message tag. For normal RPCs, this is 
     composed of the function ID (FID) and the interface ID (IID). 
     There are two special cases:
        1) Kernel messages have negative FIDs
        2) Some kernel messages have arguments embedded in the FID */

  long long xfid = fid + (iid<<FID_SIZE);
  if (iid == IID_KERNEL)
    {
      if (fid!=0)
        xfid = (((1ULL<<(globals.word_size*8-20))-1) - (fid-1))<<4;
        else xfid = 0; /* startup */
    }

  CASTExpression *xfidExpr = new CASTIntegerConstant(xfid, true);
  forAllMS(special[channel],
    {
      CMSChunk4 *chunk = (CMSChunk4*)item;
      if (chunk->specialPos == POS_PRIVILEGES)
        {
          assert(chunk->cachedInExpr);
          xfidExpr = new CASTBinaryOp("+",
            xfidExpr,
            new CASTBinaryOp("&",
              chunk->cachedInExpr->clone(),
              new CASTIntegerConstant(7))
          );
        }
    }
  );

  /* Assemble the message tag */

  int maxUntypedWords = getMaxUntypedWordsInChannel(channel);
  CASTExpression *msgTag = new CASTIntegerConstant(maxUntypedWords);
  
  if (channel == CHANNEL_IN)
    msgTag = new CASTBinaryOp("+",
      msgTag,
      new CASTBinaryOp("<<",
        new CASTBrackets(xfidExpr),
        new CASTIntegerConstant(16))
    );
  
  if (typedMRs)
    msgTag = new CASTBinaryOp("+",
      msgTag,
      new CASTBinaryOp("<<",
        new CASTIntegerConstant(typedMRs),
        new CASTIntegerConstant(6))
    );
    
  return msgTag;
}

CASTExpression *CMSConnection4::buildMemMsgSize(int channel)

{
  int fixedBytes = 0;
      
  forAllMS(mem_fixed[channel],
    {
      CMSChunk4 *chunk = (CMSChunk4*)item;
      int endsAt = chunk->offset+chunk->size;
      if ((fixedBytes<endsAt) && (chunk->flags & CHUNK_XFER_COPY))
        fixedBytes = endsAt;
    }  
  );
        
  CASTExpression *sizeExpression = new CASTIntegerConstant(fixedBytes);
  if (!mem_variable[channel]->isEmpty())
    sizeExpression = new CASTBrackets(
      new CASTBinaryOp("+",
        sizeExpression,
        new CASTIdentifier("_memvaridx"))
    );
    
  return sizeExpression;
}

CASTStatement *CMSConnection4::buildMemMsgSetup(int channel)

{
  CASTStatement *result = NULL;

  if (!mem_fixed[channel]->isEmpty() || !mem_variable[channel]->isEmpty())
    { 
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            new CASTBinaryOp(".",
              buildSourceBufferRvalue(channel),
              new CASTIdentifier("_memmsg")),
            new CASTIdentifier("len")),
          new CASTBinaryOp("<<",
            buildMemMsgSize(channel),
            new CASTIntegerConstant(10))))
      );
      
      CASTExpression *memMsgAddr = new CASTIdentifier("_memmsg");
      if (channel != CHANNEL_IN)
        {
          int displacement = getMemMsgDisplacementOnServer();
          if (displacement)
            memMsgAddr = new CASTUnaryOp("&",
              new CASTBinaryOp("->",
                memMsgAddr,
                new CASTBinaryOp(".",
                  buildChannelIdentifier(channel),
                  new CASTIndexOp(
                    new CASTIdentifier("_spacer"),
                    new CASTIntegerConstant(displacement))))
            );
        } else memMsgAddr = new CASTUnaryOp("&", memMsgAddr);
      
      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            new CASTBinaryOp(".",
              buildSourceBufferRvalue(channel),
              new CASTIdentifier("_memmsg")),
            new CASTIdentifier("ptr")),
          memMsgAddr))
      );
    }
    
  return result;
}

CASTStatement *CMSConnection4::buildClientCall(CASTExpression *target, CASTExpression *env)

{
  CASTStatement *result = NULL;

  addTo(result, buildMemMsgSetup(CHANNEL_IN));
  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      new CASTBinaryOp(".",
        buildSourceBufferRvalue(CHANNEL_IN),
        new CASTIdentifier("_msgtag")),
      buildMsgTag(CHANNEL_IN)))
  );
  addTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("L4_MsgLoad"),
      new CASTCastOp(
        new CASTDeclaration(
          new CASTTypeSpecifier(
            new CASTIdentifier("L4_Msg_t")),
          new CASTDeclarator(
            (CASTIdentifier*)NULL,
            new CASTPointer())),
        new CASTBrackets(
          new CASTUnaryOp("(void*)",
            new CASTUnaryOp("&",
              new CASTIdentifier("_pack")))),
        CASTStandardCast)))
  );
  
  bool hasOutFpages = false;
  for (int i=1;i<numChannels;i++)
    if (!fpages[i]->isEmpty())
      hasOutFpages = true;
  
  if (hasOutFpages)
    addTo(result, new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("L4_Accept"),
        new CASTBinaryOp("->",
          env->clone(),
          new CASTIdentifier("_rcv_window"))))
    );

  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      new CASTIdentifier("_result"),
      new CASTFunctionOp(
        new CASTIdentifier("L4_Ipc"),
        knitExprList(
          target,
          (options&OPTION_ONEWAY) ? new CASTIdentifier("L4_nilthread") : target->clone(),
          new CASTConditionalExpression(
            new CASTBrackets(
              new CASTBinaryOp("==", 
                env, 
                new CASTIntegerConstant(0))),
            new CASTFunctionOp(
              new CASTIdentifier("L4_Timeouts"),
              knitExprList(
                new CASTIdentifier("L4_Never"),
                new CASTIdentifier("L4_Never"))),
            new CASTBinaryOp("->", 
              env, 
              new CASTIdentifier("_timeout"))),
          new CASTUnaryOp("&",
            new CASTIdentifier("_dummy"))))))
  );
  if (!(options & OPTION_ONEWAY))
    addTo(result, new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("L4_MsgStore"),
        knitExprList(
          new CASTIdentifier("_result"),
          new CASTCastOp(
            new CASTDeclaration(
              new CASTTypeSpecifier(
                new CASTIdentifier("L4_Msg_t")),
              new CASTDeclarator(
                (CASTIdentifier*)NULL,
                new CASTPointer())),
            new CASTBrackets(
              new CASTUnaryOp("(void*)", 
                new CASTUnaryOp("&",
                  new CASTIdentifier("_pack")))),
            CASTStandardCast))))
    );

  addTo(result, buildClientUnmarshalStart());
  
  return result;    
}

CASTStatement *CMSConnection4::buildClientUnmarshalStart()

{  
  CASTStatement *result = NULL;
  if ((getOutStringItems()>0) || !fpages[CHANNEL_OUT]->isEmpty())
    {
      addTo(result, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("L4_LoadBR"),
          knitExprList(
            new CASTIntegerConstant(0),
            new CASTIntegerConstant(0))))
      );
    }

  if (!reg_variable[CHANNEL_OUT]->isEmpty())
    addTo(result,
      new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTIdentifier("_regvaridx"),
          new CASTIntegerConstant(0)))
    );
    
  if (!mem_variable[CHANNEL_OUT]->isEmpty())
    addTo(result,
      new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTIdentifier("_memvaridx"),
          new CASTIntegerConstant(0)))
    );

  return result;
}
