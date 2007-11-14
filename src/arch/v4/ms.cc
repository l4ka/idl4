#include "arch/v4.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)
#define dprint(a...) do { if (debug_mode&DEBUG_MARSHAL) print(a); } while (0)

CMSConnection4::CMSConnection4(CMSService4 *service, int numChannels, int fid, int iid, int bitsPerWord) : CMSConnection(service, numChannels, fid, iid)

{
  if (numChannels>MAX_CHANNELS) 
    panic("Channel limit reached"); 
    
  for (int i=0;i<numChannels;i++)
    {
      this->reg_fixed[i] = new CMSList();
      this->reg_variable[i] = new CMSList();
      this->mem_fixed[i] = new CMSList();
      this->mem_variable[i] = new CMSList();
      this->strings[i] = new CMSList();
      this->fpages[i] = new CMSList();
      this->special[i] = new CMSList();
    }  
       
  this->numChannels = numChannels; 
  this->bitsPerWord = bitsPerWord;
  this->chunkID = 0;
  this->options = 0;
  this->channelInitialized = CHANNEL_MASK(CHANNEL_IN);
}

CMSConnection *CMSService4::getConnection(int numChannels, int fid, int iid)

{
  CMSConnection *result = buildConnection(numChannels, fid, iid);
  connections->add(result);
  
  return result;
}

void CMSService4::finalize()

{
  assert(numStrings==0);
  assert(numDwords==0);
  
  int numBytes = 0;
  forAllMS(connections,
    {
      CMSConnection4 *thisConnection = (CMSConnection4*)item;
  
      if (thisConnection->getMaxBytesRequired()>numBytes)
        numBytes = thisConnection->getMaxBytesRequired();
    
      if (thisConnection->getMaxStringBuffersRequiredOnServer()>numStrings)
        numStrings = thisConnection->getMaxStringBuffersRequiredOnServer();
    }
  );
  
  numDwords = (numBytes+globals.word_size-1)/globals.word_size;
}

int CMSConnection4::layoutMessageRegisters(int channel)

{
  int offset = 0;
  
  forAllMS(reg_fixed[channel],
    {
      CMSChunk4 *chunk = (CMSChunk4*)item;
      assert(chunk->alignment>0);
      while (offset%chunk->alignment) offset++;
      chunk->offset = offset;
      offset += chunk->size;
    }
  );
  
  return offset;
}

int CMSConnection4::alignmentPrio(int sizeInBytes)

{
  if (sizeInBytes==1)
    return 4;
    
  if (sizeInBytes==2)
    return 3;
    
  if (sizeInBytes<=4)
    return 2;
    
  return 1;
}

void CMSConnection4::channelDump()

{
  for (int i=0;i<numChannels;i++)
    {
      if (!reg_fixed[i]->isEmpty())
        {
          dprint("  reg_fixed%d: ", i);
          forAllMS(reg_fixed[i], ((CMSChunk4*)item)->dump(false));
          dprintln();
        }  
      if (!reg_variable[i]->isEmpty())
        {
          dprint("  reg_variable%d: ", i);
          forAllMS(reg_variable[i], ((CMSChunk4*)item)->dump(false));
          dprintln();
        }  
      if (!mem_fixed[i]->isEmpty())
        {
          dprint("  mem_fixed%d: ", i);
          forAllMS(mem_fixed[i], ((CMSChunk4*)item)->dump(false));
          dprintln();
        }  
      if (!mem_variable[i]->isEmpty())
        {
          dprint("  mem_variable%d: ", i);
          forAllMS(mem_variable[i], ((CMSChunk4*)item)->dump(false));
          dprintln();
        }  
      if (!strings[i]->isEmpty())
        {
          dprint("  strings%d: ", i);
          forAllMS(strings[i], ((CMSChunk4*)item)->dump(false));
          dprintln();
        }  
      if (!fpages[i]->isEmpty())
        {
          dprint("  fpages%d: ", i);
          forAllMS(fpages[i], ((CMSChunk4*)item)->dump(false));
          dprintln();
        }  
      if (!special[i]->isEmpty())
        {
          dprint("  special%d: ", i);
          forAllMS(special[i], ((CMSChunk4*)item)->dump(false));
          dprintln();
        }  
    }  
}

#define DUMP(c, var, ofsField) \
  iterator = (CMSChunk4*)(var)->getFirstElement(); \
  \
  while (!iterator->isEndOfList()) \
    { \
      if (iterator->ofsField==UNKNOWN) \
        print("%c-: ", c); \
        else print("%c%d: ",c, iterator->ofsField); \
      iterator->dump(true); \
      iterator = (CMSChunk4*)iterator->next; \
    }  

void CMSConnection4::dump()

{
  CMSChunk4 *iterator;
  bool isFirst = true;
  
  for (int i=0;i<numChannels;i++)
    if (!(reg_fixed[i]->isEmpty() && reg_variable[i]->isEmpty() && 
          mem_fixed[i]->isEmpty() && mem_variable[i]->isEmpty() && 
          strings[i]->isEmpty() && fpages[i]->isEmpty() && special[i]->isEmpty()))
      {
        if (!isFirst) println(); 
                 else isFirst = false;
        println("Channel %d:                        ID OFFS SIZE ALGN FLAGS BUF", i);
        indent(+1);
      
        DUMP('R', reg_fixed[i], offset);
        DUMP('R', reg_variable[i], offset);
        DUMP('M', mem_fixed[i], offset);
        DUMP('M', mem_variable[i], offset);
        DUMP('S', strings[i], offset);
        DUMP('F', fpages[i], offset);
        DUMP('X', special[i], offset);
       
        indent(-1);
      }
}  

void CMSChunk4::dump(bool verbose)

{
  CASTDeclaration *decl;
  
  if (!verbose)
    {
      dprint("%d[%d] ", chunkID, size);
      return;
    }

  tabstop(4);  
  if (type)
    {
      decl = type->buildDeclaration(new CASTDeclarator(new CASTIdentifier(name)));
      decl->write();
    } else print("char %s", name);  
  tabstop(34);
  print("%2c ", "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[chunkID]);
  print("%4d ", offset);
  if (size == UNKNOWN) print(" var "); else print("%4d ", size);
  print("%4d ", alignment);
  print("%c%c%c-- ", (flags&CHUNK_XFER_COPY) ? 'x' : '-',
                     (flags&CHUNK_PINNED) ? 'p' : '-',
                     (specialPos>0) ? ('0'+specialPos) : '-');
  print("%3d", bufferIndex);
  println();
}

CASTDeclarationStatement *CMSChunk4::buildDeclarationStatement()

{
  CBEType *thisType = type;
  CASTExpression *dimension = NULL;
  
  if (type==NULL)
    {
      thisType = getType(aoiRoot->lookupSymbol("#s8", SYM_TYPE));
      if (size>1)
        dimension = new CASTIntegerConstant(size);
    } else {
             if (size>type->getFlatSize() || (flags&CHUNK_FORCE_ARRAY))
               {
                 assert((size % type->getFlatSize())==0);
                 dimension = new CASTIntegerConstant(size/type->getFlatSize());
               }  
           }    

  dprintln(" -  Declaring %s", name);
  return new CASTDeclarationStatement(
    thisType->buildMessageDeclaration(
      new CASTDeclarator(
        new CASTIdentifier(name),
        NULL,
        dimension))
  );      
}

CASTDeclarationStatement *CMSConnection4::buildMessageMembers(int channel)

{
  CASTDeclarationStatement *members = NULL;

  forAllMS(reg_fixed[channel], 
    {
      CMSChunk4 *chunk = (CMSChunk4*)item;
      addTo(members, chunk->buildDeclarationStatement());
    }    
  );
  
  if (!mem_fixed[channel]->isEmpty() || !mem_variable[channel]->isEmpty())
    addTo(members, new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTTypeSpecifier(
          new CASTIdentifier("idl4_stringitem")),
        new CASTDeclarator(
          new CASTIdentifier("_memmsg"))))
    );
    
  forAllMS(strings[channel], 
    if (((CMSChunk4*)item)->flags & CHUNK_XFER_COPY)
      addTo(members, new CASTDeclarationStatement(
        new CASTDeclaration(
          new CASTTypeSpecifier(
            new CASTIdentifier("idl4_stringitem")),
          new CASTDeclarator(
            new CASTIdentifier(item->name))))
      )
  );
    
  forAllMS(fpages[channel], 
    addTo(members, new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTTypeSpecifier(
          new CASTIdentifier("idl4_mapitem")),
        new CASTDeclarator(
          new CASTIdentifier(item->name))))
    )
  );
    
  return members;  
}

CASTIdentifier *CMSConnection4::buildChannelIdentifier(int channel)

{
  const char *structName;
  
  switch (channel)
    {
      case CHANNEL_IN  : structName = "_in";
                         break;
      case CHANNEL_OUT : structName = "_out";
                         break;
      default          : structName = mprintf("_exc%d", channel-2);
                         break;
    }                   

  return new CASTIdentifier(structName);        
}

CASTExpression *CMSConnection4::buildSourceBufferRvalue(int channel)

{
  CASTIdentifier *bufferName;
  
  if (channel==CHANNEL_IN) 
    bufferName = new CASTIdentifier("_pack");
    else bufferName = new CASTIdentifier("_par");
    
  return new CASTBinaryOp((channel==CHANNEL_IN) ? "." : "->",
    bufferName,
    buildChannelIdentifier(channel));
}

CASTExpression *CMSConnection4::buildTargetBufferRvalue(int channel)

{
  CASTIdentifier *bufferName;
  
  if (channel==CHANNEL_IN) 
    bufferName = new CASTIdentifier("_par");
    else bufferName = new CASTIdentifier("_pack");
    
  return new CASTBinaryOp((channel!=CHANNEL_IN) ? "." : "->",
    bufferName,
    buildChannelIdentifier(channel));
}

CASTExpression *CMSConnection4::buildMemmsgRvalue(int channel, bool isServer)

{
  return new CASTBinaryOp((isServer) ? "->" : ".",
    new CASTIdentifier("_memmsg"),
    buildChannelIdentifier(channel));
}

int CMSConnection4::getMemMsgDisplacementOnServer()

{
  bool hasInValues, hasOutValues;
  
  hasInValues = !mem_fixed[CHANNEL_IN]->isEmpty();
  hasOutValues = false;
  for (int i=CHANNEL_OUT;i<numChannels;i++)
    if (!mem_fixed[i]->isEmpty())
      hasOutValues = true;
      
  if (!hasInValues || !hasOutValues)
    return 0;
    
  int displacement = 0;
  forAllMS(mem_fixed[CHANNEL_IN],
    {
      CMSChunk4 *chunk = (CMSChunk4*)item;
      int endsAt = chunk->offset+chunk->size;
      if ((displacement<endsAt) && (chunk->flags & CHUNK_XFER_COPY))
        displacement = endsAt;
    }  
  );

  while (displacement % globals.word_size)
    displacement++;
    
  return displacement / globals.word_size;
}

CASTAggregateSpecifier *CMSConnection4::buildMemmsgUnion(bool isServerSide)

{
  CASTDeclarationStatement *memoryPacks = NULL;
  CBEType *mwType = msFactory->getMachineWordType();
  int displacement = getMemMsgDisplacementOnServer();
  
  for (int i=0;i<numChannels;i++)
    {
      CASTDeclarationStatement *structMembers = NULL;
      
      if (!mem_fixed[i]->isEmpty() && (i!=CHANNEL_IN) && (displacement>0) && isServerSide)
        addTo(structMembers, new CASTDeclarationStatement(
          mwType->buildDeclaration(
            new CASTDeclarator(
              new CASTIdentifier("_spacer"),
              NULL,
              new CASTIntegerConstant(displacement))))
        );
      
      forAllMS(mem_fixed[i], 
        addTo(structMembers, ((CMSChunk4*)item)->buildDeclarationStatement())
      );
        
      if (structMembers)
        addTo(memoryPacks, new CASTDeclarationStatement(
        new CASTDeclaration(
          new CASTAggregateSpecifier("struct", ANONYMOUS, structMembers),
          new CASTDeclarator(
            buildChannelIdentifier(i))))
      );
    }  
 
  if (memoryPacks)
    return new CASTAggregateSpecifier("union", ANONYMOUS, memoryPacks);
    else return NULL;    
}

CASTDeclarationStatement *CMSConnection4::buildClientStandardVars()

{
  CASTDeclarationStatement *result = NULL;

  bool needRegIndex = false, needMemIndex = false;
  for (int i=0;i<numChannels;i++)
    {
      if (!reg_variable[i]->isEmpty())
        needRegIndex = true;
      if (!mem_variable[i]->isEmpty())
        needMemIndex = true;
    }
    
  if (needRegIndex)
    addTo(result, new CASTDeclarationStatement(
      msFactory->getMachineWordType()->buildDeclaration(
          new CASTDeclarator(
            new CASTIdentifier("_regvaridx"),
            NULL,
            NULL,
            new CASTIntegerConstant(0))))
    );
  
  if (needMemIndex)
    addTo(result, new CASTDeclarationStatement(
      msFactory->getMachineWordType()->buildDeclaration(
          new CASTDeclarator(
            new CASTIdentifier("_memvaridx"),
            NULL,
            NULL,
            new CASTIntegerConstant(0))))
    );

  CASTAggregateSpecifier *memmsgUnion = buildMemmsgUnion(false);
  if (memmsgUnion)
    addTo(result, new CASTDeclarationStatement(
      new CASTDeclaration(
        memmsgUnion,
        new CASTDeclarator(
          new CASTIdentifier("_memmsg"))))
    );
    
  return result;
}

CASTDeclarationSpecifier *CMSConnection4::buildClientMsgBuf()

{
  CASTDeclarationStatement *channelPacks = NULL;

  for (int i=0;i<numChannels;i++)
    {
      CASTDeclarationStatement *structMembers = NULL;
      
      addTo(structMembers, buildMessageMembers(i));
         
      if (structMembers)
        addTo(channelPacks, new CASTDeclarationStatement(
          new CASTDeclaration(
            new CASTAggregateSpecifier("struct", ANONYMOUS, structMembers),
            new CASTDeclarator(
              buildChannelIdentifier(i))))
        );
    }  
    
  if (!channelPacks)
    return NULL;
    
  return new CASTAggregateSpecifier("union", new CASTIdentifier("_buf"), channelPacks);
}

CASTStatement *CMSConnection4::buildClientLocalVars(CASTIdentifier *key)

{
  CASTDeclarationStatement *result = buildClientStandardVars();

  addTo(result, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTTypeSpecifier(
        new CASTIdentifier("L4_MsgTag_t")),
      new CASTDeclarator(
        new CASTIdentifier("_result"))))
  );

  addTo(result, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTTypeSpecifier(
        new CASTIdentifier("L4_ThreadId_t")),
      new CASTDeclarator(
        new CASTIdentifier("_dummy"))))
  );

  addTo(result, new CASTDeclarationStatement(
    new CASTDeclaration(
      buildClientMsgBuf(),
      new CASTDeclarator(
        new CASTIdentifier("_pack"))))
  );

  return result;
}

CMSChunk4 *CMSConnection4::findChunk(CMSList *list, ChunkID chunkID)

{
  CMSBase *iterator = list->getFirstElement();
  
  while (!iterator->isEndOfList())
    if (((CMSChunk*)iterator)->chunkID != chunkID)
      iterator = iterator->next;
      else return (CMSChunk4*)iterator;
      
  return NULL;
}

CASTStatement *CMSConnection4::buildServerStandardDecls(CASTIdentifier *key)

{
  CBEType *mwType = msFactory->getMachineWordType();
  CASTDeclarationStatement *unionMembers = NULL;
  CASTStatement *result = NULL;

  for (int i=0;i<numChannels;i++)
    {
      CASTDeclarationStatement *structMembers = NULL;
      
      if (i!=CHANNEL_IN)
        addTo(structMembers, new CASTDeclarationStatement(
          mwType->buildDeclaration(
            new CASTDeclarator(
              new CASTIdentifier("_spacer"),
              NULL,
              new CASTIntegerConstant(64))))
        );      

      addTo(structMembers, buildMessageMembers(i));
      
      addTo(unionMembers, new CASTDeclarationStatement(
        new CASTDeclaration(
          new CASTAggregateSpecifier("struct", ANONYMOUS, structMembers),
          new CASTDeclarator(
            buildChannelIdentifier(i))))
      );
    }  

  addTo(unionMembers, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTAggregateSpecifier("struct", ANONYMOUS, 
        knitStmtList(
          new CASTDeclarationStatement(
            msFactory->getMachineWordType()->buildDeclaration(
              new CASTDeclarator(
                new CASTIdentifier("_spacer"),
                NULL,
                new CASTIntegerConstant(128)))),
          new CASTDeclarationStatement(
            new CASTDeclaration(
              new CASTTypeSpecifier(
                new CASTIdentifier("idl4_inverse_stringitem")),
              new CASTDeclarator(
                new CASTIdentifier("_str"),
                NULL,
                new CASTIntegerConstant(16)))),
          new CASTDeclarationStatement(
            msFactory->getMachineWordType()->buildDeclaration(
              new CASTDeclarator(
                new CASTIdentifier("_acceptor")))))),
      new CASTDeclarator(
        new CASTIdentifier("_buf"))))
  );

  CASTDeclarationSpecifier *spec = NULL;
  addTo(spec, new CASTStorageClassSpecifier("typedef"));
  addTo(spec, new CASTAggregateSpecifier("union", ANONYMOUS, unionMembers));
  addTo(result, new CASTDeclarationStatement(new CASTDeclaration(spec, new CASTDeclarator(buildServerParamID(key)))));
  
  CASTAggregateSpecifier *memmsgUnion = buildMemmsgUnion(true);
  if (memmsgUnion)
    {
      CASTDeclarationSpecifier *spec2 = NULL;
      addTo(spec2, new CASTStorageClassSpecifier("typedef"));
      addTo(spec2, memmsgUnion);
      
      CASTIdentifier *memmsgIdentifier = key->clone();
      memmsgIdentifier->addPrefix("_memmsg_");
      
      addTo(result, new CASTSpacer());
      addTo(result, new CASTDeclarationStatement(new CASTDeclaration(spec2, new CASTDeclarator(memmsgIdentifier))));
    }
    
  return result;
}

CBEType *CMSConnection4::getWrapperReturnType()

{
  return msFactory->getMachineWordType();
}

CASTStatement *CMSConnection4::buildServerDeclarations(CASTIdentifier *key)

{
  CASTStatement *result = buildServerStandardDecls(key);
  CBEType *mwType = msFactory->getMachineWordType();
  
  CASTIdentifier *wrapperIdentifier = key->clone();
  wrapperIdentifier->addPrefix("service_");
  
  CASTDeclarationStatement *wrapperDecl = new CASTDeclarationStatement(
    getWrapperReturnType()->buildDeclaration( 
      new CASTDeclarator(
        wrapperIdentifier,
        buildWrapperParams(key),
        NULL, 
        NULL, 
        NULL, 
        buildWrapperAttributes()))
  );
  
  addWithLeadingSpacerTo(result, wrapperDecl);

  return result;  
}

CASTStatement *CMSConnection4::buildServerReplyDeclarations(CASTIdentifier *key)

{
  CASTStatement *result = new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTAggregateSpecifier("struct",
        ANONYMOUS,
        buildMessageMembers(CHANNEL_OUT)),
      new CASTDeclarator(
        buildChannelIdentifier(CHANNEL_OUT)))
  );

  result = new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTAggregateSpecifier("struct", 
        new CASTIdentifier("_reply_buffer"), 
        result),
      new CASTDeclarator(
        new CASTIdentifier("_buf")))
  );
  
  addTo(result, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTAggregateSpecifier("struct", 
        new CASTIdentifier("_reply_buffer")),
      new CASTDeclarator(
        new CASTIdentifier("_par"),
        new CASTPointer(1),
        NULL,
        new CASTUnaryOp("&",
          new CASTIdentifier("_buf")))))
  );

  if (!mem_fixed[CHANNEL_OUT]->isEmpty() || !mem_variable[CHANNEL_OUT]->isEmpty())
    {
      CASTIdentifier *memmsgIdentifier = key->clone();
      memmsgIdentifier->addPrefix("_memmsg_");

      addTo(result, new CASTDeclarationStatement(
        new CASTDeclaration(
          new CASTTypeSpecifier(memmsgIdentifier),
          new CASTDeclarator(
            new CASTIdentifier("_membuf"))))
      );

      addTo(result, new CASTDeclarationStatement(
        new CASTDeclaration(
          new CASTTypeSpecifier(memmsgIdentifier->clone()),
          new CASTDeclarator(
            new CASTIdentifier("_memmsg"),
            new CASTPointer(1),
            NULL,
            new CASTUnaryOp("&",
              new CASTIdentifier("_membuf")))))
      );
      
      if (!mem_variable[CHANNEL_OUT]->isEmpty())
        addTo(result, new CASTDeclarationStatement(
          msFactory->getMachineWordType()->buildDeclaration(
              new CASTDeclarator(
                new CASTIdentifier("_memvaridx"),
                NULL,
                NULL,
                new CASTIntegerConstant(0))))
        );
    }
  
  if (!reg_variable[CHANNEL_OUT]->isEmpty())
    addTo(result, new CASTDeclarationStatement(
      msFactory->getMachineWordType()->buildDeclaration(
          new CASTDeclarator(
            new CASTIdentifier("_regvaridx"),
            NULL,
            NULL,
            new CASTIntegerConstant(0))))
    );

  return result;  
}

CASTStatement *CMSConnection4::buildServerReply()

{
  CASTStatement *result = NULL;

  if (!mem_fixed[CHANNEL_OUT]->isEmpty() || !mem_variable[CHANNEL_OUT]->isEmpty())
    addTo(result, buildMemMsgSetup(CHANNEL_OUT));
  
  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      new CASTBinaryOp(".",
        new CASTIdentifier("_buf"),
        new CASTBinaryOp(".",
          new CASTIdentifier("_out"),
          new CASTIdentifier("_msgtag"))),
      buildMsgTag(CHANNEL_OUT)))
  );
  
  addTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("L4_MsgLoad"),
      new CASTUnaryOp("(L4_Msg_t *)",
        new CASTBrackets(
          new CASTUnaryOp("(void*)",
            new CASTIdentifier("_par"))))))
  );
  
  addTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("L4_Send_Timeout"),
      knitExprList(
        new CASTIdentifier("_client"),
        new CASTIdentifier("L4_ZeroTime"))))
  );
  
  return result;
}

CASTStatement *CMSConnection4::buildServerLocalVars(CASTIdentifier *key)

{
  CASTDeclarationStatement *result = NULL;

  bool needRegIndex = false, needMemIndex = false, needMemmsg = false;
  for (int i=0;i<numChannels;i++)
    {
      if (!reg_variable[i]->isEmpty())
        needRegIndex = true;
      if (!mem_variable[i]->isEmpty())
        needMemIndex = true;
      if (!mem_fixed[i]->isEmpty() || !mem_variable[i]->isEmpty())
        needMemmsg = true;
    }
    
  if (needRegIndex)
    addTo(result, new CASTDeclarationStatement(
      msFactory->getMachineWordType()->buildDeclaration(
          new CASTDeclarator(
            new CASTIdentifier("_regvaridx"),
            NULL,
            NULL,
            new CASTIntegerConstant(0))))
    );
  
  if (needMemIndex)
    addTo(result, new CASTDeclarationStatement(
      msFactory->getMachineWordType()->buildDeclaration(
          new CASTDeclarator(
            new CASTIdentifier("_memvaridx"),
            NULL,
            NULL,
            new CASTIntegerConstant(0))))
    );

  CASTIdentifier *memmsgIdentifier = key->clone();
  memmsgIdentifier->addPrefix("_memmsg_");

  if (needMemmsg)
    addTo(result, new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTTypeSpecifier(memmsgIdentifier),
        new CASTDeclarator(
          new CASTIdentifier("_memmsg"),
          new CASTPointer(1),
          NULL,
          new CASTCastOp(
            new CASTDeclaration(
              new CASTTypeSpecifier(memmsgIdentifier),
              new CASTDeclarator(
                (CASTIdentifier*)NULL, 
                new CASTPointer(1))),
            new CASTBinaryOp(".",
              new CASTBinaryOp(".",
                new CASTBinaryOp("->",
                  new CASTIdentifier("_par"),
                  new CASTIdentifier("_buf")),
                new CASTIndexOp(  
                  new CASTIdentifier("_str"),
                  new CASTIntegerConstant(15))),
              new CASTIdentifier("ptr")),
            CASTStandardCast))))
    );

  return result;
}

CASTExpression *CMSConnection4::buildServerCallerID()

{
  return new CASTIdentifier("_caller");
}

CASTStatement *CMSConnection4::buildServerAbort()

{
  return new CASTReturnStatement(new CASTIntegerConstant(-1UL, true));
}

CASTStatement *CMSConnection4::buildServerBackjump(int channel, CASTExpression *environment)

{
  CASTStatement *result = NULL;

  addTo(result, buildMemMsgSetup(CHANNEL_OUT));

  int untypedWords = getMaxUntypedWordsInChannel(channel);
  int specialWords = 0;
  if (!mem_fixed[channel]->isEmpty() || !mem_variable[channel]->isEmpty())
    specialWords = 2;
  forAllMS(fpages[channel], specialWords += 2);
  forAllMS(strings[channel], if (((CMSChunk4*)item)->flags & CHUNK_XFER_COPY) specialWords += 2);

  CASTExpression *mr0 = new CASTBinaryOp("+",
    new CASTBrackets(
      new CASTBinaryOp("<<",
        new CASTIntegerConstant(specialWords),
        new CASTIntegerConstant(6))),
    new CASTIntegerConstant(untypedWords)
  );

  if (channel>1)
    mr0 = new CASTBinaryOp("+", 
      new CASTBinaryOp("<<",
        new CASTBinaryOp(".",
          environment,
          new CASTIdentifier("_action")),
        new CASTIntegerConstant(16)),
      mr0
    );
  
  addTo(result, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      new CASTBinaryOp(".",
        buildSourceBufferRvalue(CHANNEL_OUT),
        new CASTIdentifier("_msgtag")),
    mr0))
  );
  
  addTo(result, new CASTReturnStatement(
    new CASTIntegerConstant(untypedWords + specialWords))
  );
  
  return result;
}

CASTIdentifier *CMSConnection4::buildServerParamID(CASTIdentifier *key)

{
  CASTIdentifier *unionIdentifier = key->clone();
  unionIdentifier->addPrefix("_param_");
  return unionIdentifier;
}

CASTDeclaration *CMSConnection4::buildWrapperParams(CASTIdentifier *key)

{
  CASTDeclaration *wrapperParams = NULL;
  
  addTo(wrapperParams, msFactory->getThreadIDType()->buildDeclaration(
    new CASTDeclarator(new CASTIdentifier("_caller")))
  );

  addTo(wrapperParams, new CASTDeclaration(
    new CASTTypeSpecifier(buildServerParamID(key)),
    new CASTDeclarator(new CASTIdentifier("_par"), new CASTPointer(1)))
  );  
  
  return wrapperParams;
}

CASTBase *CMSConnection4::buildServerWrapper(CASTIdentifier *key, CASTCompoundStatement *compound)

{
  CASTIdentifier *wrapperIdentifier = key->clone();
  wrapperIdentifier->addPrefix("service_");
  
  return getWrapperReturnType()->buildDeclaration( 
    new CASTDeclarator(wrapperIdentifier, 
      buildWrapperParams(key),
        NULL, 
        NULL, 
        NULL, 
        buildWrapperAttributes()), 
    compound
  );
}

CASTAttributeSpecifier *CMSConnection4::buildWrapperAttributes()

{
  return NULL;
}

CASTStatement *CMSConnection4::buildServerMarshalInit()

{
  CASTStatement *result = NULL;

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

CASTStatement *CMSConnection4::buildServerTestStructural()

{
  return NULL;
}

int CMSConnection4::getOutStringItems()

{
  int result = 0;
  
  forAllMS(strings[CHANNEL_OUT], 
    if (((CMSChunk4*)item)->flags & CHUNK_XFER_COPY)
      result++;
  );
  
  if (!mem_fixed[CHANNEL_OUT]->isEmpty() || !mem_variable[CHANNEL_OUT]->isEmpty())
    result++;
    
  return result;
}

void CMSConnection4::setOption(int option)

{
  this->options |= option;
  if (option == OPTION_ONEWAY)
    channelInitialized |= CHANNEL_ALL_OUT;
}

CASTStatement *CMSConnection4::buildClientInit()

{
  bool needsRcvWindow = !fpages[CHANNEL_OUT]->isEmpty();
  int numBuffers = getOutStringItems();
  CASTStatement *result = NULL;

  int used_BRs = 0;

  forAllMS(strings[CHANNEL_OUT],
    {
      CMSChunk4 *chunk = (CMSChunk4*)item;
      
      if (chunk->flags & CHUNK_XFER_COPY)
        {
          CASTExpression *addrBR;
          
          if (!chunk->targetBuffer)
            {
              int lengthBound = SERVER_BUFFER_SIZE / chunk->contentType->getFlatSize();

              addrBR = new CASTBrackets(
                new CASTUnaryOp("(void*)",
                  chunk->contentType->buildBufferAllocation(new CASTIntegerConstant(lengthBound)))
              );
            }
          else
            {
              addrBR = chunk->targetBuffer->clone();
            }

          char *buffer = (char *) malloc(20);
          snprintf(buffer, 20, "_rcv_buf%d", chunk->offset);
          addTo(result, new CASTDeclarationStatement(
            new CASTDeclaration(
              new CASTTypeSpecifier("L4_Word_t"),
              new CASTDeclarator(
                new CASTIdentifier(buffer),
                NULL,
                NULL,
                new CASTUnaryOp("(L4_Word_t)", addrBR))))
          );
        }  
    }
  );

  if ((numBuffers>0) || needsRcvWindow)
    {
      CASTExpression *br0 = new CASTIntegerConstant((numBuffers>0) ? 1 : 0);
      if (needsRcvWindow)
        br0 = new CASTBinaryOp("|",
          br0,
          new CASTBinaryOp(".",
            new CASTBinaryOp("->",
              new CASTIdentifier("_env"),
              new CASTIdentifier("_rcv_window")),
            new CASTIdentifier("raw"))
        );
          
      addTo(result, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("L4_LoadBR"),
          knitExprList(
            new CASTIntegerConstant(0),
            br0)))
      );

      used_BRs = max(used_BRs, (0) +1);

    }

  forAllMS(strings[CHANNEL_OUT],
    {
      CMSChunk4 *chunk = (CMSChunk4*)item;
      
      if (chunk->flags & CHUNK_XFER_COPY)
        {
          CASTExpression *sizeBR;
          
          if (!chunk->targetBuffer)
            {
              int lengthBound = SERVER_BUFFER_SIZE / chunk->contentType->getFlatSize();

              assert(chunk->contentType);
              CASTExpression *rcvSize = new CASTIntegerConstant(lengthBound);
              if (chunk->contentType->getFlatSize()>1)
                rcvSize = new CASTBinaryOp("*",
                  rcvSize,
                  new CASTFunctionOp(
                    new CASTIdentifier("sizeof"),
                    new CASTDeclarationExpression(
                      chunk->contentType->buildDeclaration(NULL)))
                );
                       
              sizeBR = rcvSize;
            }
          else
            {
              assert(chunk->targetBufferSize);
              sizeBR = chunk->targetBufferSize->clone();
            }
                   
          sizeBR = new CASTBinaryOp("<<",
            sizeBR,
            new CASTIntegerConstant(10)
          );

          if ((chunk->offset+1) < numBuffers)
            sizeBR = new CASTBinaryOp("|",
              new CASTBrackets(sizeBR),
              new CASTIntegerConstant(1)
            );
          

          addTo(result, new CASTExpressionStatement(
            new CASTFunctionOp(
              new CASTIdentifier("L4_LoadBR"),
              knitExprList(
                new CASTIntegerConstant(chunk->offset*2 + 1),
                sizeBR)))
          );

          used_BRs = max(used_BRs, (chunk->offset*2 + 1) +1);

          char *buffer = (char *) malloc(20);
          snprintf(buffer, 20, "_rcv_buf%d", chunk->offset);
          addTo(result, new CASTExpressionStatement(
            new CASTFunctionOp(
              new CASTIdentifier("L4_LoadBR"),
              knitExprList(
                new CASTIntegerConstant(2*(chunk->offset + 1)),
                new CASTIdentifier(buffer))))
          );

          used_BRs = max(used_BRs, (2*(chunk->offset + 1)) +1);

        }  
    }
  );

  if (!mem_fixed[CHANNEL_OUT]->isEmpty() || !mem_variable[CHANNEL_OUT]->isEmpty())
    {
      int maxSize = 0;
      forAllMS(mem_fixed[CHANNEL_OUT], maxSize += ((CMSChunk4*)item)->size);
      forAllMS(mem_variable[CHANNEL_OUT], maxSize += ((CMSChunk4*)item)->size);
      
      addTo(result, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("L4_LoadBR"),
          knitExprList(
            new CASTIntegerConstant(1),
            new CASTBinaryOp("<<",
              new CASTIntegerConstant(maxSize),
              new CASTIntegerConstant(10)))))
      );  

          used_BRs = max(used_BRs, (1) +1);

      addTo(result, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("L4_LoadBR"),
          knitExprList(
            new CASTIntegerConstant(2),
            new CASTUnaryOp("(L4_Word_t)",
              new CASTUnaryOp("&",
                new CASTIdentifier("_memmsg"))))))
      );  

          used_BRs = max(used_BRs, (2) +1);

    }

  /* Save number of destroyed BRs, those will be restored by idl4_msgbuf_sync() in server loop. */
  if (used_BRs>0)
    {
      addTo(result, new CASTSpacer());
      addTo(result, new CASTComment("Save number of destroyed BRs, those will be restored by idl4_msgbuf_sync() in server loop."));
      addWithTrailingSpacerTo(result, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("idl4_set_counter_minimum"),
          knitExprList(
            new CASTIntegerConstant(used_BRs))))
      );
    }

  return result;
}

CASTStatement *CMSConnection4::buildClientFinish()

{
  CASTStatement *result = NULL;

  forAllMS(strings[CHANNEL_OUT],
    {
      CMSChunk4 *chunk = (CMSChunk4*)item;
     
      if ((chunk->flags & CHUNK_XFER_COPY) && (!chunk->targetBuffer))
        addTo(result, new CASTExpressionStatement(
          new CASTFunctionOp(
            new CASTIdentifier("CORBA_free"),
            new CASTFunctionOp(
              new CASTIdentifier("idl4_get_buffer_addr"),
              new CASTIntegerConstant(chunk->offset))))
        );  
    }
  );

  return result;
}

void CMSConnection4::reset()

{
  for (int i=0;i<numChannels;i++)
    {
      forAllMS(reg_fixed[i], ((CMSChunk4*)item)->reset());
      forAllMS(reg_variable[i], ((CMSChunk4*)item)->reset());
      forAllMS(mem_fixed[i], ((CMSChunk4*)item)->reset());
      forAllMS(mem_variable[i], ((CMSChunk4*)item)->reset());
      forAllMS(strings[i], ((CMSChunk4*)item)->reset());
      forAllMS(fpages[i], ((CMSChunk4*)item)->reset());
    }
}

void CMSChunk4::reset()

{
}

int CMSConnection4::getMaxBytesRequired()

{
  int maxBytes = 0;
  
  for (int i=0;i<numChannels;i++)
    {
      forAllMS(mem_fixed[i],
        {
          CMSChunk4 *chunk = (CMSChunk4*)item;
          maxBytes = max(maxBytes, chunk->offset + chunk->size);
        }  
      );  
    }    
    
  return maxBytes;
}

int CMSConnection4::getMaxStringBuffersRequiredOnServer()

{
  int maxBuffers = 0;
  
  for (int i=0;i<numChannels;i++)
    {
      int buffersInChannel = 0;
      forAllMS(strings[i], buffersInChannel = max(buffersInChannel, 1+((CMSChunk4*)item)->bufferIndex));
      if (!mem_fixed[i]->isEmpty() || !mem_variable[i]->isEmpty())
        buffersInChannel++;
      maxBuffers = max(maxBuffers, buffersInChannel);
    }
    
  return maxBuffers;
}

void CMSFactory4::initRootScope(CAoiRootScope *rootScope)

{
  CAoiModule *idl4 = getBuiltinScope(rootScope, "idl4");
  rootScope->submodules->add(idl4);
  idl4->constants->add(getBuiltinConstant(rootScope, idl4, "startup", 0));
  idl4->constants->add(getBuiltinConstant(rootScope, idl4, "interrupt", 1));
  idl4->constants->add(getBuiltinConstant(rootScope, idl4, "pagefault", 2));
  idl4->constants->add(getBuiltinConstant(rootScope, idl4, "preemption", 3));
  idl4->constants->add(getBuiltinConstant(rootScope, idl4, "sysexc", 4));
  idl4->constants->add(getBuiltinConstant(rootScope, idl4, "archexc", 5));
  idl4->constants->add(getBuiltinConstant(rootScope, idl4, "sigma0rpc", 6));
  idl4->constants->add(getBuiltinConstant(rootScope, idl4, "iopagefault", 8));
   
}

CASTExpression *CMSConnection4::buildClientCallSucceeded()

{
  return new CASTFunctionOp(
    new CASTIdentifier("L4_IpcSucceeded"),
    new CASTIdentifier("_result")
  );
}

CASTStatement *CMSConnection4::buildClientResultAssignment(CASTExpression *environment, CASTExpression *defaultValue)

{
  CASTStatement *userCodeAssignment = NULL;
  
  if (!(options&OPTION_ONEWAY))
    userCodeAssignment = new CASTExpressionStatement(
    new CASTBinaryOp("=",
      new CASTUnaryOp("*",
        new CASTUnaryOp("(L4_Word_t*)",
          environment->clone())),
      defaultValue)
  );

  return new CASTCompoundStatement(
    new CASTIfStatement(
      new CASTUnaryOp("!",
        new CASTFunctionOp(
          new CASTIdentifier("L4_IpcSucceeded"),
          new CASTIdentifier("_result"))),
      new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTUnaryOp("*",
            new CASTUnaryOp("(L4_Word_t*)",
              environment->clone())),
          new CASTBinaryOp("+", 
            new CASTIdentifier("CORBA_SYSTEM_EXCEPTION"),
            new CASTBinaryOp("<<",
              new CASTFunctionOp(new CASTIdentifier("L4_ErrorCode")),
              new CASTIntegerConstant(8))))),
      userCodeAssignment)
  );
}

CASTExpression *CMSConnection4::buildLabelExpr()

{
  return new CASTFunctionOp(
    new CASTIdentifier("L4_Label"),
    new CASTIdentifier("_result")
  );
}

CMSChunk4 *CMSConnection4::getFirstTypedItem(int channel)

{
  forAllMS(strings[channel],
    if (((CMSChunk4*)item)->flags & CHUNK_XFER_COPY)
      return (CMSChunk4*)item
  );
    
  if (!fpages[channel]->isEmpty())
    return (CMSChunk4*)fpages[channel]->getFirstElement();
    
  return NULL;
}

CASTStatement *CMSConnection4::buildPositionTest(int channel)

{
  CMSChunk4 *firstTypedItem = getFirstTypedItem(channel);

  if (!firstTypedItem)
    return NULL;
    
  int assumedPos = (getMaxUntypedWordsInChannel(channel)+1) * globals.word_size;
  CASTExpression *actualPosition = new CASTBinaryOp("-",
    new CASTUnaryOp("(unsigned)&",
      new CASTBinaryOp(".",
        buildSourceBufferRvalue(channel),
        new CASTIdentifier(firstTypedItem->name))),
    new CASTUnaryOp("(unsigned)&",
      buildSourceBufferRvalue(channel))
  );
  
  return new CASTIfStatement(
    new CASTBinaryOp("!=",
      actualPosition,
      new CASTIntegerConstant(assumedPos)),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(
          new CASTStringConstant(false, mprintf("Alignment error in channel %d: First typed item %s is at %%d (should be %d)", 
                                                channel, firstTypedItem->name, assumedPos)),
          actualPosition->clone()
        )))
  );
}
