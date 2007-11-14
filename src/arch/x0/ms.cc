#include "globals.h"
#include "cast.h"
#include "arch/x0.h"

// TODO: OPTIMIZATIONS
//   - multiple values in a register
//   - sort struct members according to size

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)
#define dprint(a...) do { if (debug_mode&DEBUG_MARSHAL) print(a); } while (0)

CMSConnectionX::CMSConnectionX(CMSServiceX *service, int numChannels, int fid, int iid, int numRegs, int bitsPerWord) : CMSConnection(service, numChannels, fid, iid)

{
  if (numChannels>MAX_CHANNELS) 
    panic("Channel limit reached"); 
    
  for (int i=0;i<numChannels;i++)
    {
      this->dopes[i] = new CMSList();
      this->registers[i] = new CMSList();
      this->dwords[i] = new CMSList();
      this->vars[i] = new CMSList();
      this->strings[i] = new CMSList();
    }  
       
  this->numChannels = numChannels; 
  this->numRegs = numRegs;
  this->bitsPerWord = bitsPerWord;
  this->chunkID = 0;
  this->finalized = false;
  this->service = service;
  this->options = 0;
  this->channelInitialized = CHANNEL_MASK(CHANNEL_IN);
}

void CMSConnectionX::setOption(int option)

{
  this->options |= option;
  if (option == OPTION_ONEWAY)
    channelInitialized |= CHANNEL_ALL_OUT;
}

bool CMSConnectionX::isWorthString(int minSize, int typSize, int maxSize)

{
  if ((typSize!=UNKNOWN) && (typSize>120))
    return true;
    
  if ((maxSize==UNKNOWN) || (maxSize>200))
    return true;

  return false;
}

int CMSConnectionX::calculatePriority(int channels)

{
  int prio = PRIO_IN_FIXED;
        
  if (HAS_OUT(channels)) 
    prio = PRIO_OUT_FIXED;
  if (HAS_IN_AND_OUT(channels))
    prio = PRIO_INOUT_FIXED;  
    
  return prio;
}  

CMSChunkX *CMSChunkX::findSubchunk(ChunkID chunkID)

{
  if (chunkID == this->chunkID)
    return this;
    
  return NULL;
}

CMSChunkX *CMSSharedChunkX::findSubchunk(ChunkID chunkID)

{
  if (chunk1->findSubchunk(chunkID))
    return chunk1->findSubchunk(chunkID);
    
  if (chunk2->findSubchunk(chunkID))
    return chunk2->findSubchunk(chunkID);
    
  return NULL;
}

void CMSChunkX::setClientPosition(int clientOffset, int clientIndex)

{
  this->clientOffset = clientOffset;
  this->clientIndex = clientIndex;
}

void CMSChunkX::setServerPosition(int serverOffset, int serverIndex)

{
  this->serverOffset = serverOffset;
  this->serverIndex = serverIndex;
}

void CMSSharedChunkX::setClientPosition(int clientOffset, int clientIndex)

{
  CMSChunkX::setClientPosition(clientOffset, clientIndex);
  chunk1->setClientPosition(clientOffset, clientIndex);
  chunk2->setClientPosition(clientOffset + chunk1->size, clientIndex);
}

void CMSSharedChunkX::setServerPosition(int serverOffset, int serverIndex)

{
  CMSChunkX::setServerPosition(serverOffset, serverIndex);
  chunk1->setServerPosition(serverOffset, serverIndex);
  chunk2->setServerPosition(serverOffset + chunk1->size, serverIndex);
}

CASTDeclarationStatement *CMSChunkX::buildDeclarationStatement()

{
  CBEType *thisType = type;
  CASTExpression *dimension = NULL;
  
  if (type==NULL)
    {
      thisType = getType(aoiRoot->lookupSymbol("#s8", SYM_TYPE));
      if (size>1)
        dimension = new CASTIntegerConstant(size);
    }    
  
  dprintln(" -  Declaring %s", name);
  return new CASTDeclarationStatement(
    thisType->buildDeclaration(
      new CASTDeclarator(
        new CASTIdentifier(name),
        NULL,
        dimension))
  );      
}

CASTDeclarationStatement *CMSChunkX::buildRegisterBuffer(int channel, CASTExpression *reg)

{
  CASTExpression *initializer = NULL;

  if (!temporaryBuffer)
    return NULL;
    
  if (HAS_IN(channels) && HAS_OUT(channels) && (channel==CHANNEL_OUT))
    return NULL;
    
  if (HAS_IN(channels))
    {
      initializer = reg;
      
      int bytesToShift = serverOffset%4;
      if (bytesToShift)
        initializer = new CASTBinaryOp(">>",
          initializer,
          new CASTIntegerConstant(bytesToShift*8)
        );
      if (size<4)
        initializer = new CASTBinaryOp("&", 
          new CASTBrackets(
            initializer),
          new CASTHexConstant(~(~(1<<(size*8))+1))
        );
    }  
  
  if (type->isScalar() || !HAS_IN(channels))
    {
      if (initializer)
        initializer = type->buildTypeCast(0, initializer);
        
      return new CASTDeclarationStatement(
        type->buildDeclaration(
          new CASTDeclarator(
            temporaryBuffer->clone(),
            NULL,
            NULL,
            initializer))
      );
    } else {
             if (initializer)
               initializer = new CASTArrayInitializer(
                 new CASTLabeledExpression(
                   new CASTIdentifier("raw"),
                   initializer)
               );
    
             CASTTypeSpecifier *rawTypeSpecifier = NULL;
             switch (size)
               {
                 case 1 : rawTypeSpecifier = new CASTTypeSpecifier(new CASTIdentifier("unsigned char"));
                          break;
                 case 2 : rawTypeSpecifier = new CASTTypeSpecifier(new CASTIdentifier("unsigned short"));
                          break;
                 default: rawTypeSpecifier = new CASTTypeSpecifier(new CASTIdentifier("unsigned int"));
                          break;
               }
             
             return new CASTDeclarationStatement(
               new CASTDeclaration(
                 new CASTAggregateSpecifier(
                   "union",
                   NULL,
                   knitStmtList(
                     new CASTDeclarationStatement(
                       type->buildDeclaration(
                         new CASTDeclarator(
                           new CASTIdentifier("typed")))),
                     new CASTDeclarationStatement(
                       new CASTDeclaration(
                         rawTypeSpecifier,
                         new CASTDeclarator(
                           new CASTIdentifier("raw")))))),
                 new CASTDeclarator(
                   temporaryBuffer->clone(),
                   NULL,
                   NULL,
                   initializer)));
           }  
}

CASTIdentifier *CMSConnectionX::buildChannelIdentifier(int channel)

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

CASTExpression *CMSConnectionX::buildSourceBufferRvalue(int channel)

{
  if (channel==CHANNEL_IN) 
    return new CASTBinaryOp(".",
      new CASTIdentifier("_pack"),
      buildChannelIdentifier(channel)
    );  
    else return new CASTBinaryOp("->",
      new CASTIdentifier("msgbuf"),
      buildChannelIdentifier(channel)
    );
}

CASTExpression *CMSConnectionX::buildTargetBufferRvalue(int channel)

{
  if (channel!=CHANNEL_IN) 
    return new CASTBinaryOp(".",
      new CASTIdentifier("_pack"),
      buildChannelIdentifier(channel)
    );  
    else return new CASTBinaryOp("->",
      new CASTIdentifier("msgbuf"),
      buildChannelIdentifier(channel)
    );
}

CASTStatement *CMSConnectionX::buildClientLocalVarsIndep()

{
  CASTDeclarationStatement *result = NULL;

  bool hasVarbuf = false;
  for (int i=0;i<numChannels;i++)
    if (!vars[i]->isEmpty())
      hasVarbuf = true;
      
  if (hasVarbuf)
    addTo(result, new CASTDeclarationStatement(
      msFactory->getMachineWordType()->buildDeclaration(
        new CASTDeclarator(
          new CASTIdentifier("_varidx"),
          NULL,
          NULL,
          new CASTIntegerConstant(0))))
    );
  
  CASTDeclarationStatement *channelPacks = NULL;
  for (int i=0;i<numChannels;i++)
    if (!isShortIPC(i))
      addTo(channelPacks, new CASTDeclarationStatement(
        new CASTDeclaration(
          new CASTAggregateSpecifier("struct", ANONYMOUS, buildMessageMembers(i, false)),
          new CASTDeclarator(
            buildChannelIdentifier(i))))
      );
    
  if (channelPacks)
    addTo(result, new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTAggregateSpecifier("union", ANONYMOUS, channelPacks),
        new CASTDeclarator(
          new CASTIdentifier("_pack"))))
    );
    
  return result;
}

CMSChunkX *CMSConnectionX::findChunk(CMSList *list, ChunkID chunkID)

{
  CMSBase *iterator = list->getFirstElement();
  
  while (!iterator->isEndOfList())
    {
      CMSChunkX *chunk = ((CMSChunkX*)iterator)->findSubchunk(chunkID);
      if (chunk)
        return chunk;
        
      iterator = iterator->next;
    }  
      
  return NULL;
}

CASTStatement *CMSConnectionX::buildServerDeclarations(CASTIdentifier *key)

{
  CASTDeclarationStatement *unionMembers = NULL;
  CASTStatement *result = NULL;

  for (int i=0;i<numChannels;i++)
    {
      CASTDeclarationStatement *structMembers = buildMessageMembers(i, true);
      
      if (structMembers)
        addTo(unionMembers, new CASTDeclarationStatement(
          new CASTDeclaration(
            new CASTAggregateSpecifier("struct", ANONYMOUS, structMembers),
            new CASTDeclarator(
              buildChannelIdentifier(i))))
        );
    }  
  
  CASTDeclarationSpecifier *spec = NULL;
  addTo(spec, new CASTStorageClassSpecifier("typedef"));
  addTo(spec, new CASTAggregateSpecifier("union", ANONYMOUS, unionMembers));
    
  CASTIdentifier *unionIdentifier = key->clone();
  unionIdentifier->addPrefix("_param_");

  addTo(result, new CASTDeclarationStatement(
    new CASTDeclaration(spec, 
      new CASTDeclarator(unionIdentifier)))
  );
  
  addTo(result, new CASTSpacer());
  
  /************************************************************************/
  
  CASTIdentifier *wrapperIdentifier = key->clone();
  wrapperIdentifier->addPrefix("service_");
  
  CBEType *mwType = msFactory->getMachineWordType();
  addTo(result, new CASTDeclarationStatement(
    mwType->buildDeclaration( 
      new CASTDeclarator(
        wrapperIdentifier,
        buildWrapperParams(key))))
  );

  return result;  
}

CASTDeclaration *CMSConnectionX::buildWrapperParams(CASTIdentifier *key)

{
  CBEType *mwType = msFactory->getMachineWordType();
  CASTDeclaration *result = NULL;
  CASTIdentifier *unionIdentifier = key->clone();
  
  unionIdentifier->addPrefix("_param_");
  addTo(result, new CASTDeclaration(
    new CASTTypeSpecifier(new CASTIdentifier("l4_threadid_t")),
    new CASTDeclarator(new CASTIdentifier("_caller"))));
  addTo(result, new CASTDeclaration(
    new CASTTypeSpecifier(unionIdentifier->clone()),
    new CASTDeclarator(new CASTIdentifier("msgbuf"), new CASTPointer())));
  for (int i=0;i<numRegs;i++)
    addTo(result, mwType->buildDeclaration(
      new CASTDeclarator(
        new CASTIdentifier(mprintf("w%d", i)), 
        new CASTPointer()))
    );
  
  return result;
}

CASTExpression *CMSConnectionX::buildStackTopExpression()

{
  return new CASTIdentifier("_par");
}

CASTExpression *CMSConnectionX::buildCallerExpression()

{
  return new CASTIdentifier("_caller");
}

CASTStatement *CMSConnectionX::buildServerAbort()

{
  return new CASTReturnStatement(new CASTIntegerConstant(-1UL));
}

CASTStatement *CMSConnectionX::buildServerBackjump(int channel, CASTExpression *environment)

{
  int firstXferByte, lastXferByte, firstXferString, numXferStrings;
  analyzeChannel(channel, true, true, &firstXferByte, &lastXferByte, &firstXferString, &numXferStrings);

  CASTStatement *responseStatements = NULL;
  if (!(options&OPTION_ONEWAY))
    {
      int fpageFlag = (hasFpagesInChannel(channel) ? 2 : 0);
      int svrOutputOffset = (firstXferByte==UNKNOWN) ? 0 : firstXferByte - 12;

      addTo(responseStatements, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            buildSourceBufferRvalue(channel),
            new CASTIdentifier("_send_dope")),  
          new CASTConditionalExpression(
            new CASTBrackets(
              new CASTBinaryOp("==",
                new CASTBinaryOp(".",
                  environment->clone(),
                  new CASTIdentifier("_action")),
                new CASTIntegerConstant(0))),
            buildSendDope(channel),
            new CASTIntegerConstant(0))))
      );

      CMSChunkX *excChunk = findChunk(registers[channel], 0);
      if (excChunk)
        excChunk->setCachedInput(0, new CASTBinaryOp(".",
          environment,
          new CASTIdentifier("_action"))
        );

      CMSChunkX *chunk = (CMSChunkX*)registers[channel]->getFirstElement();
      while (!chunk->isEndOfList())
        {
          for (int nr = 0; nr < ((chunk->size>4) ? 2 : 1); nr++)
            {
              CASTExpression *cachedInput = chunk->getCachedInput(nr);
              if (cachedInput)
                addTo(responseStatements,
                  new CASTExpressionStatement(
                    new CASTBinaryOp("=",
                      buildRegisterVar(chunk->serverIndex + nr),
                      cachedInput))
                );
                else warning("Missing in expr for chunk %d (nr %d) in channel %d", chunk->chunkID, nr, channel);
            }  
      
          chunk = (CMSChunkX*)chunk->next;
        }

      if ((firstXferByte!=UNKNOWN) || (numXferStrings>0))
        {
          /* The reply is a long message */

          int dopeOffset = 9999;
          forAllMS(dopes[channel],
            {
              CMSChunkX *chunk = (CMSChunkX*)item;
              if (chunk->serverOffset < dopeOffset)
                dopeOffset = chunk->serverOffset;
            }
          );
          assert(dopeOffset != 9999);

          if (numXferStrings>0)
            {
              int bytesInBuffer = firstXferString - (dopeOffset + 12);
              int sizeDope = (bytesInBuffer<<11) + (numXferStrings<<8);

              assert((bytesInBuffer%4)==0);
              addTo(responseStatements,
                new CASTExpressionStatement(
                  new CASTBinaryOp("=",
                    new CASTBinaryOp(".",
                      new CASTBinaryOp("->",
                        new CASTIdentifier("msgbuf"),
                        buildChannelIdentifier(channel)),
                      new CASTIdentifier("_size_dope")),  
                    new CASTHexConstant(sizeDope)))
              );
            }

          forAllMS(dwords[channel],
            {
              CMSChunkX *chunk = (CMSChunkX*)item;
              if (chunk->flags & CHUNK_INIT_ZERO)
                {
                  for (int i=0;i<2;i++)
                    addTo(responseStatements, new CASTExpressionStatement(
                      new CASTBinaryOp("=",
                        new CASTIndexOp(
                          new CASTBinaryOp(".",
                            buildSourceBufferRvalue(channel),
                            new CASTIdentifier("__mapPad")),
                          new CASTIntegerConstant(i)),  
                        new CASTIntegerConstant(0)))
                    );      
                }
            }
          );

          assert(!dopeOffset || (dopeOffset>=8));
          addTo(responseStatements, 
            new CASTReturnStatement(
              new CASTBinaryOp("+",
                new CASTUnaryOp("(int)",
                  new CASTIdentifier("msgbuf")),
                new CASTIntegerConstant(dopeOffset + fpageFlag)))
          );
        } else addTo(responseStatements, new CASTReturnStatement(new CASTIntegerConstant(fpageFlag)));

    } else addTo(responseStatements, new CASTReturnStatement(new CASTIntegerConstant(-1UL)));

  return new CASTCompoundStatement(responseStatements);
}

CASTStatement *CMSConnectionX::buildServerLocalVars(CASTIdentifier *key)

{
  CASTStatement *result = NULL;
 
  bool hasVarbuf = false;
  for (int i=0;i<numChannels;i++)
    if (!vars[i]->isEmpty())
      hasVarbuf = true;
      
  if (hasVarbuf)
    addTo(result, new CASTDeclarationStatement(
      msFactory->getMachineWordType()->buildDeclaration(
        new CASTDeclarator(
          new CASTIdentifier("_varidx"),
          NULL,
          NULL,
          new CASTIntegerConstant(0))))
    );

  forAllMS(registers[CHANNEL_IN],
    addTo(result, ((CMSChunkX*)item)->buildRegisterBuffer(CHANNEL_IN, buildRegisterVar(((CMSChunkX*)item)->serverIndex)))
  );

  forAllMS(registers[CHANNEL_OUT],
    addTo(result, ((CMSChunkX*)item)->buildRegisterBuffer(CHANNEL_OUT, buildRegisterVar(((CMSChunkX*)item)->serverIndex)))
  );
    
  return result;  
}

int CMSServiceX::getServerSizeDope(int byteOffset)

{
  int dwordOffset = byteOffset>>2;
  
  assert((byteOffset%4)==0);
  assert(dwordOffset<=numDwords);
  
  int numStrings = 0;
  forAllMS(connections, numStrings = max(numStrings, ((CMSConnectionX*)item)->getMaxStringsAcceptedByServer()));
  
  return (((numDwords-dwordOffset-3)<<13) + (numStrings<<8));
}  

CMSConnection *CMSServiceX::getConnection(int numChannels, int fid, int iid)

{
  CMSConnection *result = buildConnection(numChannels, fid, iid);
  connections->add(result);
  
  return result;
}

void CMSServiceX::finalize()

{
  assert(numDwords==0);
  assert(numStrings==0);
  
  int numBytes = 0;
  
  forAllMS(connections,
    {
      CMSConnectionX *thisConnection = (CMSConnectionX*)item;
  
      if (thisConnection->getMaxBytesRequired(true)>numBytes)
        numBytes = thisConnection->getMaxBytesRequired(true);
    
      if (thisConnection->getMaxStringBuffersRequiredOnServer()>numStrings)
        numStrings = thisConnection->getMaxStringBuffersRequiredOnServer();
    }
  );

  /* If no copy dwords are used (e.g. only indirect strings), numBytes can
     be zero. In this case, we make sure that there is at least enough
     room for two message buffers (one for receiving, one for sending) */

  if (numBytes < 9*4)
    numBytes = 9*4;

  numDwords = (numBytes+3)/4;

  forAllMS(connections, ((CMSConnectionX*)item)->finalize(numDwords*4));
}

CASTBase *CMSConnectionX::buildServerWrapper(CASTIdentifier *key, CASTCompoundStatement *compound)

{
  CBEType *mwType = msFactory->getMachineWordType();

  CASTIdentifier *unionIdentifier = key->clone();
  unionIdentifier->addPrefix("_param_");

  CASTIdentifier *wrapperIdentifier = key->clone();
  wrapperIdentifier->addPrefix("service_");
  
  return mwType->buildDeclaration( 
    new CASTDeclarator(wrapperIdentifier, buildWrapperParams(key)), compound);
}

CASTExpression *CMSConnectionX::buildServerCallerID()

{
  return new CASTIdentifier("_caller");
}

CASTStatement *CMSConnectionX::buildServerMarshalInit()

{
  if (!vars[CHANNEL_OUT]->isEmpty())
    return new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTIdentifier("_varidx"),
        new CASTIntegerConstant(0)));
        
  return NULL;
}

CASTStatement *CMSConnectionX::buildClientInit()

{
  CASTStatement *result = NULL;
  
  forAllMS(strings[CHANNEL_OUT],
    {
      CMSChunkX *chunk = (CMSChunkX*)item;
      
      if (chunk->flags & CHUNK_XFER_COPY)
        {
          CASTExpression *rcvAddrExpr = new CASTBinaryOp(".",
            new CASTBinaryOp(".",
              buildTargetBufferRvalue(CHANNEL_OUT),
              new CASTIdentifier(chunk->name)),
            new CASTIdentifier("rcv_addr")
          );

          CASTExpression *rcvSizeExpr = new CASTBinaryOp(".",
            new CASTBinaryOp(".",
              buildTargetBufferRvalue(CHANNEL_OUT),
              new CASTIdentifier(chunk->name)),
            new CASTIdentifier("rcv_size")
          );

          if (chunk->targetBuffer)
            {
              assert(chunk->targetBufferSize);
              addTo(result, new CASTExpressionStatement(
                new CASTBinaryOp("=", 
                  rcvAddrExpr, 
                  chunk->targetBuffer->clone()))
              );
              addTo(result, new CASTExpressionStatement(
                new CASTBinaryOp("=",
                  rcvSizeExpr,
                  chunk->targetBufferSize->clone()))
              );
            } else {
                     int lengthBound = (chunk->sizeBound==UNKNOWN) ? SERVER_BUFFER_SIZE / chunk->contentType->getFlatSize() : chunk->sizeBound;
                     if ((chunk->sizeBound==UNKNOWN) && (globals.warnings&WARN_PREALLOC))
                       warning("Preallocating infinite out string");
      
                     assert(chunk->contentType);
                     addTo(result, new CASTExpressionStatement(
                     new CASTBinaryOp("=",
                       rcvAddrExpr,
                       chunk->contentType->buildBufferAllocation(new CASTIntegerConstant(lengthBound))))
                     );  
          
                     CASTExpression *rcvSize = new CASTIntegerConstant(lengthBound);
                     if (chunk->contentType->getFlatSize()>1)
                       rcvSize = new CASTBinaryOp("*",
                         rcvSize,
                         new CASTFunctionOp(
                           new CASTIdentifier("sizeof"),
                           new CASTDeclarationExpression(
                             chunk->contentType->buildDeclaration(NULL)))
                       );
          
                     addTo(result, new CASTExpressionStatement(
                       new CASTBinaryOp("=",
                         rcvSizeExpr,
                         rcvSize))
                     );  
                   }  
        }  
    }
  );
      
  return result;
}

CASTStatement *CMSConnectionX::buildClientFinish()

{
  CASTStatement *result = NULL;
  
  forAllMS(strings[CHANNEL_OUT],
    {
      CMSChunkX *chunk = (CMSChunkX*)item;
     
      if ((chunk->flags & CHUNK_XFER_COPY) && (!chunk->targetBuffer))
        addTo(result, new CASTExpressionStatement(
          new CASTFunctionOp(
            new CASTIdentifier("CORBA_free"),
              new CASTBinaryOp(".",
                new CASTBinaryOp(".",
                  buildTargetBufferRvalue(CHANNEL_OUT),
                  new CASTIdentifier(chunk->name)),
                new CASTIdentifier("rcv_addr"))))
        );  
    }
  );
      
  return result;
}

const char *CMSServiceX::getShortRegisterName(int no)

{
  switch (no)
    {
      case 0 : return "d";
      case 1 : return "b";
      case 2 : return "D";
    }
  return mprintf("(unknown register: %d)", no);
}

const char *CMSServiceX::getLongRegisterName(int no)

{
  switch (no)
    {
      case 0 : return "edx";
      case 1 : return "ebx";
      case 2 : return "edi";
    }
  return mprintf("(unknown register: %d)", no);
}

CASTAsmConstraint *CMSServiceX::getThreadidInConstraints(CASTExpression *rvalue)

{
  return new CASTAsmConstraint("S", rvalue);
}

void CMSConnectionX::reset()

{
  for (int i=0;i<numChannels;i++)
    {
      forAllMS(registers[i], ((CMSChunkX*)item)->reset());
      forAllMS(dwords[i], ((CMSChunkX*)item)->reset());
      forAllMS(vars[i], ((CMSChunkX*)item)->reset());
      forAllMS(strings[i], ((CMSChunkX*)item)->reset());
    }
}

void CMSChunkX::reset()

{
  for (int i=0;i<2;i++)
    cachedInExpr[i] = cachedOutExpr[i] = NULL;
}

CASTExpression *CMSChunkX::getCachedInput(int nr)

{
  if (!cachedInExpr[nr])
    return NULL;
    
  char *storageEquivalent;
  switch (size)
    {
      case 1 : storageEquivalent = "unsigned char";break;
      case 2 : storageEquivalent = "unsigned short";break;
      default: storageEquivalent = "unsigned int";break;
    }
    
  if (type->isScalar() || cachedInExpr[nr]->isConstant())
    return new CASTUnaryOp(mprintf("(%s)", storageEquivalent), cachedInExpr[nr]->clone());
    
  return new CASTUnaryOp(
    mprintf("*(%s*)&", storageEquivalent), 
    cachedInExpr[nr]->clone()
  );
}

CASTExpression *CMSSharedChunkX::getCachedInput(int nr)

{
  return new CASTBinaryOp("+",
      new CASTBinaryOp("<<",
        new CASTBrackets(
          chunk2->getCachedInput(nr)),
        new CASTIntegerConstant(chunk1->size * 8)),
    chunk1->getCachedInput(nr)
  );
}

CASTExpression *CMSChunkX::getCachedOutput(int nr)

{
  return cachedOutExpr[nr];
}

CASTExpression *CMSSharedChunkX::getCachedOutput(int nr)

{
  return new CASTStringConstant(false, "blobb");
}

void CMSChunkX::setCachedInput(int nr, CASTExpression *rvalue)

{
  assert(!cachedInExpr[nr]);
  cachedInExpr[nr] = rvalue;
}

void CMSChunkX::setCachedOutput(int nr, CASTExpression *lvalue)

{
  assert(!cachedOutExpr[nr]);
  cachedOutExpr[nr] = lvalue;
}

void CMSSharedChunkX::setCachedInput(int nr, CASTExpression *rvalue)

{
  panic("Cannot set cached input for shared chunk %d", chunkID);
}

void CMSSharedChunkX::setCachedOutput(int nr, CASTExpression *lvalue)

{
  panic("Cannot set cached output for shared chunk %d", chunkID);
}

CASTStatement *CMSServiceX::buildServerDeclarations(CASTIdentifier *prefix, int vtableSize, int itableSize, int ktableSize)

{
  CASTIdentifier *msgbufSize = prefix->clone();
  CASTIdentifier *strbufSize = prefix->clone();
  CASTIdentifier *rcvDope = prefix->clone();
  CASTIdentifier *fidMask = prefix->clone();
  CASTIdentifier *iidMask = prefix->clone();
  
  msgbufSize->addPostfix("_MSGBUF_SIZE");
  strbufSize->addPostfix("_STRBUF_SIZE");
  rcvDope->addPostfix("_RCV_DOPE");
  fidMask->addPostfix("_FID_MASK");
  iidMask->addPostfix("_IID_MASK");
  assert(numDwords>=3);
  assert(ktableSize==0);
  
  CASTStatement *result = NULL;
  addTo(result, new CASTPreprocDefine(msgbufSize, new CASTIntegerConstant(numDwords-3)));
  addTo(result, new CASTPreprocDefine(strbufSize, new CASTIntegerConstant(numStrings)));
  addTo(result, new CASTPreprocDefine(rcvDope, new CASTHexConstant(getServerSizeDope(0))));
  addTo(result, new CASTPreprocDefine(fidMask, new CASTHexConstant(vtableSize-1)));
  if (itableSize>0)
    addTo(result, new CASTPreprocDefine(iidMask, new CASTHexConstant(itableSize-1)));
  
  return result;
}

CASTExpression *CMSConnectionX::buildRegisterVar(int index)

{
  return new CASTUnaryOp("*",
    new CASTIdentifier(mprintf("w%d", index))
  );
}

CASTExpression *CMSConnectionX::buildClientCallSucceeded()

{
  return new CASTBinaryOp("==",
    new CASTBinaryOp(".",
      new CASTBinaryOp(".",
        new CASTIdentifier("_msgDope"),
        new CASTIdentifier("md")),
      new CASTIdentifier("error_code")),
    new CASTIntegerConstant(0)
  );
}

CASTStatement *CMSConnectionX::buildClientResultAssignment(CASTExpression *environment, CASTExpression *defaultValue)

{
  return new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("idl4_internal_set_exception"),
      knitExprList(
        environment,
        new CASTIdentifier("_msgDope"),
        defaultValue))
  );
}

CASTStatement *CMSConnectionX::buildServerReplyDeclarations(CASTIdentifier *key)

{
  CASTStatement *result = NULL;
  
  if (!isShortIPC(CHANNEL_OUT))
    {
      result = new CASTDeclarationStatement(
        new CASTDeclaration(
          new CASTAggregateSpecifier("struct",
            ANONYMOUS,
            buildMessageMembers(CHANNEL_OUT, false)),
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
            new CASTIdentifier("msgbuf"),
            new CASTPointer(1),
            NULL,
            new CASTUnaryOp("&",
              new CASTIdentifier("_buf")))))
      );

      if (!vars[CHANNEL_OUT]->isEmpty())
        addTo(result, new CASTDeclarationStatement(
          msFactory->getMachineWordType()->buildDeclaration(
            new CASTDeclarator(
              new CASTIdentifier("_varidx"),
              NULL,
              NULL,
              new CASTIntegerConstant(0))))
        );
    }

  addTo(result, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTTypeSpecifier("l4_msgdope_t"),
        new CASTDeclarator(
          new CASTIdentifier("_msgDope"))))
  );

  return result;
}

CASTStatement *CMSConnectionX::buildServerDopeAssignment()

{
  CASTStatement *result = NULL;

  int firstXferByte, lastXferByte, firstXferString, numXferStrings;
  analyzeChannel(CHANNEL_OUT, false, true, &firstXferByte, &lastXferByte, &firstXferString, &numXferStrings);

  int fpageFlag = (hasFpagesInChannel(CHANNEL_OUT) ? 2 : 0);
  int svrOutputOffset = (firstXferByte==UNKNOWN) ? 0 : firstXferByte - 12;

  CASTExpression *msgTag = NULL;
  if ((firstXferByte!=UNKNOWN) || (numXferStrings>0))
    {
      /* The reply is a long message */

      addTo(result, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            buildSourceBufferRvalue(CHANNEL_OUT),
            new CASTIdentifier("_send_dope")),  
          buildSendDope(CHANNEL_OUT)))
      );

      if (numXferStrings>0)
        {
          int bytesInBuffer = firstXferString - 12;
          int sizeDope = (bytesInBuffer<<11) + (numXferStrings<<8);

          assert((bytesInBuffer%4)==0);
          addTo(result,
            new CASTExpressionStatement(
              new CASTBinaryOp("=",
                new CASTBinaryOp(".",
                  buildSourceBufferRvalue(CHANNEL_OUT),
                  new CASTIdentifier("_size_dope")),  
                new CASTHexConstant(sizeDope)))
          );
        }

      forAllMS(dwords[CHANNEL_OUT],
        {
          CMSChunkX *chunk = (CMSChunkX*)item;
          if (chunk->flags & CHUNK_INIT_ZERO)
            {
              for (int i=0;i<2;i++)
                addTo(result, new CASTExpressionStatement(
                  new CASTBinaryOp("=",
                    new CASTIndexOp(
                      new CASTBinaryOp(".",
                        buildSourceBufferRvalue(CHANNEL_OUT),
                        new CASTIdentifier("__mapPad")),
                      new CASTIntegerConstant(i)),  
                    new CASTIntegerConstant(0)))
                );      
            }
        }
      );
    } 

  return result;
}

CASTStatement *CMSConnectionX::buildServerReply()

{
  CASTStatement *result = buildServerDopeAssignment();

  CMSChunkX *excChunk = findChunk(registers[CHANNEL_OUT], 0);
  if (excChunk)
    excChunk->setCachedInput(0, new CASTIntegerConstant(0));

  CASTExpression *args = NULL;
  addTo(args, new CASTIdentifier("_client"));

  if (!isShortIPC(CHANNEL_OUT))
    {
      CASTExpression *msgTag = new CASTIdentifier("msgbuf");
      if (hasFpagesInChannel(CHANNEL_OUT))
        msgTag = new CASTBrackets(
          new CASTBinaryOp("+",
            new CASTUnaryOp("(int)",
              msgTag),
            new CASTIntegerConstant(2))
        );
      addTo(args, new CASTUnaryOp("(void*)", msgTag));
    } else {
             if (hasFpagesInChannel(CHANNEL_OUT))
               addTo(args, new CASTIdentifier("IDL4_IPC_REG_FPAGE"));
               else addTo(args, new CASTIdentifier("IDL4_IPC_REG_MSG"));
           }

  int currentReg = 0;
  forAllMS(registers[CHANNEL_OUT],
    {
      CMSChunkX *chunk = (CMSChunkX*)item;
      
      assert(currentReg<=chunk->clientIndex);
      while (currentReg<chunk->clientIndex)
        {
          addTo(args, new CASTIntegerConstant(0));
          currentReg++;
        }
      
      for (int nr = 0; nr < ((chunk->size>4) ? 2 : 1); nr++)
        {
          CASTExpression *cachedInput = chunk->getCachedInput(nr);
        
          if (cachedInput)
            addTo(args, cachedInput);
            else warning("Chunk %d (%s): Not initialized in channel %d (subexpr %d)", chunk->chunkID, item->name, CHANNEL_IN, nr);

          currentReg++;
        }
    }
  );
  
  assert(currentReg<=numRegs);
  while (currentReg<numRegs)
    addTo(args, new CASTIntegerConstant(0));

  CASTExpression *timeoutArgs = NULL;
  for (int i=0; i<6; i++)
    addTo(timeoutArgs, new CASTIntegerConstant((i==0 || i==2) ? 0 : 15));

  addTo(args, new CASTFunctionOp(
    new CASTIdentifier("L4_IPC_TIMEOUT"),
    timeoutArgs)
  );
  
  addTo(args, new CASTUnaryOp("&", new CASTIdentifier("_msgDope")));
  
  addTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier(getIPCBindingName(true)),
      args))
  );

  return result;
}

