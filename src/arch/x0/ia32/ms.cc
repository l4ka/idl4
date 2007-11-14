#include "arch/x0.h"

CMSService *CMSFactoryIX::getLocalService()

{ 
  if (globals.flags&FLAG_LIPC) 
    return new CMSServiceIXL(); 
    else return getService(); 
}

CASTStatement *CMSConnectionIX::buildServerDeclarations(CASTIdentifier *key)

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
  
  CBEType *voidType = getType(aoiRoot->lookupSymbol("#void", SYM_TYPE));
  CASTAttributeSpecifier *attrs = NULL;
  addTo(attrs, new CASTAttributeSpecifier(
    new CASTIdentifier("noreturn"))
  );
  addTo(attrs, new CASTAttributeSpecifier(
    new CASTIdentifier("regparm"),
    new CASTIntegerConstant(3))
  );
  
  addTo(result, new CASTDeclarationStatement(
    voidType->buildDeclaration( 
      new CASTDeclarator(
        wrapperIdentifier,
        buildWrapperParams(key),
        NULL, NULL, NULL,
        attrs
        )))
  );

  return result;  
}

CASTDeclaration *CMSConnectionIX::buildWrapperParams(CASTIdentifier *key)

{
  CBEType *mwType = msFactory->getMachineWordType();
  CASTIdentifier *unionIdentifier = key->clone();
  CASTDeclaration *result = NULL;
  
  unionIdentifier->addPrefix("_param_");
  addTo(result, mwType->buildDeclaration(
    new CASTDeclarator(new CASTIdentifier("w1"))));
  addTo(result, mwType->buildDeclaration(
    new CASTDeclarator(new CASTIdentifier("w0"))));
  addTo(result, new CASTDeclaration(
    new CASTTypeSpecifier(new CASTIdentifier("l4_threadid_t")),
    new CASTDeclarator(new CASTIdentifier("_caller"))));
  addTo(result, new CASTDeclaration(
    new CASTTypeSpecifier(unionIdentifier->clone()),
    new CASTDeclarator(new CASTIdentifier("_par")))
  );  
  
  return result;
}

CASTStatement *CMSConnectionIX::buildServerAbort()

{
  CASTAsmInstruction *noResponseInstructions = NULL;
  addTo(noResponseInstructions, new CASTAsmInstruction(aprintf("lea -4+%%0, %%%%esp")));
  addTo(noResponseInstructions, new CASTAsmInstruction(aprintf("movl $-1, %%%%eax")));
  addTo(noResponseInstructions, new CASTAsmInstruction(aprintf("ret")));
  
  CASTAsmConstraint *noResponseConstraints = NULL;
  addTo(noResponseConstraints, new CASTAsmConstraint("m", buildStackTopExpression()));

  CASTStatement *result = NULL;
  addTo(result, new CASTBlockComment("do not reply"));
  addWithTrailingSpacerTo(result, new CASTAsmStatement(noResponseInstructions, noResponseConstraints, NULL, NULL, new CASTConstOrVolatileSpecifier("__volatile__")));
  addTo(result, new CASTBlockComment("prevent gcc from generating return code"));
  addTo(result, new CASTWhileStatement(new CASTIntegerConstant(1)));

  return result;
}

CASTStatement *CMSConnectionIX::buildServerBackjump(int channel, CASTExpression *environment)

{
  bool needsSendDope = false;
  CASTAsmInstruction *jumpInstructions = NULL;

  addTo(jumpInstructions, new CASTAsmInstruction(aprintf("lea -4+%%0, %%%%esp")));

  int firstXferByte, lastXferByte, firstXferString, numXferStrings;
  analyzeChannel(channel, true, true, &firstXferByte, &lastXferByte, &firstXferString, &numXferStrings);

  CASTAsmConstraint *inConstraints = NULL;
  addTo(inConstraints, new CASTAsmConstraint("m", buildStackTopExpression()));
  
  if ((firstXferByte!=UNKNOWN) || (numXferStrings>0))
    {
      /* The reply is a long message */
    
      int fpageFlag = (hasFpagesInChannel(channel) ? 2 : 0);
      int svrOutputOffset = (firstXferByte==UNKNOWN) ? 4 : (firstXferByte - (12 + (numRegs*4)));

      assert(svrOutputOffset>=0);
      addTo(jumpInstructions, new CASTAsmInstruction(aprintf("lea %d(%%%%esp), %%%%eax", svrOutputOffset + 4 + fpageFlag + 4*service->getServerLocalVars())));
  
      // We only need the size dope if there are strings to be sent. It only has
      // to be written if we don't use the primary dope.
      // If we have _only_ strings to send, the send dope must not be placed directly
      // before the first string dope, because this might cause other string buffers
      // to be overwritten. Also, we cannot use output offset 0 because the size
      // dope needs to be preserved. Thus, we use 4.

      if ((svrOutputOffset>0) && (numXferStrings>0))
        {
          int bytesInBuffer = ((firstXferByte==UNKNOWN) ? firstXferString - (12 + 4) : (numRegs*4 + firstXferString - firstXferByte));
          assert((bytesInBuffer%4)==0);
          int sizeDope = (bytesInBuffer<<11) + (numXferStrings<<8);
          addTo(jumpInstructions, new CASTAsmInstruction(aprintf("movl $0x%X, %d(%%%%eax)", sizeDope, 4 - fpageFlag)));
        }
        
      addTo(jumpInstructions, new CASTAsmInstruction(aprintf("movl %%%%ecx, %d(%%%%eax)", 8 - fpageFlag)));
      needsSendDope = true;
    } else {
             /* The reply is a short message, or the operation is oneway */
             
             if (!(options&OPTION_ONEWAY))
               {
                 if (hasFpagesInChannel(channel))
                   {
                     addTo(inConstraints, new CASTAsmConstraint("a", 
                       new CASTConditionalExpression(
                         new CASTBrackets(
                           new CASTBinaryOp("==",
                             new CASTBinaryOp(".",
                               environment,
                               new CASTIdentifier("_action")),
                             new CASTIntegerConstant(0))),
                         new CASTIntegerConstant(2),
                         new CASTIntegerConstant(0)))
                     );     
                   } else addTo(inConstraints, new CASTAsmConstraint("a", new CASTIntegerConstant(0)));
               } else addTo(inConstraints, new CASTAsmConstraint("a", new CASTIntegerConstant(-1UL)));
           }
           
  addTo(jumpInstructions, new CASTAsmInstruction(aprintf("ret")));

  if (needsSendDope)
    addTo(inConstraints, new CASTAsmConstraint("c", 
      buildSendDope(channel))
    );
    
  addTo(inConstraints, service->getThreadidInConstraints(
    buildCallerExpression())
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
            addTo(inConstraints, 
              new CASTAsmConstraint(
                ((CMSServiceX*)service)->getShortRegisterName(chunk->serverIndex + nr), 
                cachedInput)
            );
            else warning("Missing in expr for chunk %d (nr %d) in channel %d", chunk->chunkID, nr, channel);
        }  
      
      chunk = (CMSChunkX*)chunk->next;
    }

  /* Some elements of the message may require initialization; for example, the
     map delimiters must be set to zero */

  CASTStatement *initStatements = NULL;
  forAllMS(dwords[channel],
    {
      CMSChunkX *chunk = (CMSChunkX*)item;
      if (chunk->flags & CHUNK_INIT_ZERO)
        {
          for (int i=0;i<2;i++)
            addTo(initStatements, new CASTExpressionStatement(
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
  if (initStatements)
    addTo(initStatements, new CASTSpacer());

  /***************************************************************************/

  CASTStatement *result = NULL;
  addTo(result, initStatements);
  addTo(result, new CASTAsmStatement(jumpInstructions, inConstraints, NULL, NULL, new CASTConstOrVolatileSpecifier("__volatile__")));
  
  if (initStatements)
    result = new CASTCompoundStatement(result);
  
  return result;
}

CASTBase *CMSConnectionIX::buildServerWrapper(CASTIdentifier *key, CASTCompoundStatement *compound)

{
  CBEType *voidType = getType(aoiRoot->lookupSymbol("#void", SYM_TYPE));
  CBEType *mwType = msFactory->getMachineWordType();

  CASTIdentifier *unionIdentifier = key->clone();
  unionIdentifier->addPrefix("_param_");

  CASTIdentifier *wrapperIdentifier = key->clone();
  wrapperIdentifier->addPrefix("service_");
  
  return voidType->buildDeclaration( 
    new CASTDeclarator(wrapperIdentifier, buildWrapperParams(key)), compound);
}

void CMSConnectionIX::optimize()

{
  CMSConnectionX::optimize();
  
  /* Old-style restrictions... most of these could go */
  
  for (int i=0;i<numChannels;i++)
    {
      forAllMS(registers[i],
        {
          CMSChunkX *chunk = (CMSChunkX*)item;
          if (!(!strcmp(item->name, "__fid") || !strcmp(item->name, "__eid") || (chunk->prio==PRIO_MAP)))
            {
              chunk->flags &= (~CHUNK_ALLOC_SERVER);
              if (chunk->isShared())
                {
                  CMSSharedChunkX *schunk = (CMSSharedChunkX*)chunk;
                  schunk->chunk1->flags &= (~CHUNK_ALLOC_SERVER);
                  schunk->chunk2->flags &= (~CHUNK_ALLOC_SERVER);
                }
            }  
        }
      ); 
      forAllMS(dopes[i],
        {
          CMSChunkX *chunk = (CMSChunkX*)item;
          if (!(!strcmp(item->name, "_esp") || !strcmp(item->name, "_ebp")))
            chunk->flags &= ~CHUNK_ALLOC_SERVER;
        }
      );
    }
}

CASTExpression *CMSConnectionIX::buildSourceBufferRvalue(int channel)

{
  CASTIdentifier *bufferName;
  
  if (channel==CHANNEL_IN) 
    bufferName = new CASTIdentifier("_pack");
    else bufferName = new CASTIdentifier("_par");
    
  return new CASTBinaryOp(".",
    bufferName,
    buildChannelIdentifier(channel));
}

CASTExpression *CMSConnectionIX::buildTargetBufferRvalue(int channel)

{
  CASTIdentifier *bufferName;
  
  if (channel==CHANNEL_IN) 
    bufferName = new CASTIdentifier("_par");
    else bufferName = new CASTIdentifier("_pack");
    
  return new CASTBinaryOp(".",
    bufferName,
    buildChannelIdentifier(channel));
}

CASTExpression *CMSConnectionIX::buildRegisterVar(int index)

{
  return new CASTIdentifier(mprintf("w%d", index));
}

CASTExpression *CMSConnectionIX::buildClientCallSucceeded()

{
  return new CASTBinaryOp("==",
    new CASTBinaryOp("&",
      new CASTIdentifier("_msgDope"),
      new CASTHexConstant(0xF0)),
    new CASTIntegerConstant(0)
  );
}

CASTStatement *CMSConnectionIX::buildClientResultAssignment(CASTExpression *environment, CASTExpression *defaultValue)

{
  CASTAsmConstraint *inconst3 = NULL;
  addTo(inconst3, new CASTAsmConstraint("a", new CASTIdentifier("_msgDope")));
  if (!(options&OPTION_ONEWAY))
    addTo(inconst3, new CASTAsmConstraint("D", defaultValue));
  
  CASTAsmConstraint *outconst3 = NULL;
  addTo(outconst3, new CASTAsmConstraint("=a", 
    new CASTUnaryOp("*(int*)", environment))
  );
  
  CASTAsmInstruction *inst3 = NULL;
  addTo(inst3, new CASTAsmInstruction(aprintf("andl $0xF0, %%%%eax")));
  addTo(inst3, new CASTAsmInstruction(aprintf("shl $4, %%%%eax")));
  addTo(inst3, new CASTAsmInstruction(aprintf("setnz %%%%al")));
  if (!(options&OPTION_ONEWAY))
    addTo(inst3, new CASTAsmInstruction(aprintf("cmovz %%%%edi, %%%%eax")));

  return new CASTAsmStatement(inst3, inconst3, outconst3, NULL, new CASTConstOrVolatileSpecifier("__volatile__"));
}

CASTStatement *CMSConnectionIX::buildServerReplyDeclarations(CASTIdentifier *key)

{
  CASTStatement *result = NULL;
  
  addTo(result, new CASTDeclarationStatement(
    msFactory->getMachineWordType()->buildDeclaration(
      new CASTDeclarator(
        new CASTIdentifier("_dummy"))))
  );

  if (!isShortIPC(CHANNEL_OUT))
    {
      addTo(result, new CASTDeclarationStatement(
        new CASTDeclaration(
          new CASTAggregateSpecifier("struct", 
            new CASTIdentifier("_reply_buffer"), 
            new CASTDeclarationStatement(
              new CASTDeclaration(
                new CASTAggregateSpecifier("struct",
                  ANONYMOUS,
                  buildMessageMembers(CHANNEL_OUT, false)),
                new CASTDeclarator(
                  buildChannelIdentifier(CHANNEL_OUT))))),
          new CASTDeclarator(
            new CASTIdentifier("_par"))))
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

  return result;
}

CASTStatement *CMSConnectionIX::buildServerReply()

{
  CASTStatement *result = NULL;

  addTo(result, buildServerDopeAssignment());

  CMSChunkX *excChunk = findChunk(registers[CHANNEL_OUT], 0);
  if (excChunk)
    excChunk->setCachedInput(0, new CASTIntegerConstant(0));

  CASTAsmInstruction *inst1 = NULL;
  addTo(inst1, new CASTAsmInstruction(aprintf("push %%%%ebp")));

  // Receive descriptor and Timeout
  
  if (!(options & OPTION_FASTCALL))
    {
      addTo(inst1, new CASTAsmInstruction(aprintf("mov $-1, %%%%ebp")));
      addTo(inst1, new CASTAsmInstruction(aprintf("mov $0xf0, %%%%ecx")));
    } else {
             addTo(inst1, new CASTAsmInstruction(aprintf("push $0xf0")));
             addTo(inst1, new CASTAsmInstruction(aprintf("push $-1")));
           }

  if (isShortIPC(CHANNEL_OUT))
    {
      if (hasFpagesInChannel(CHANNEL_OUT))
        addTo(inst1, new CASTAsmInstruction(aprintf("mov $2, %%%%eax")));
        else addTo(inst1, new CASTAsmInstruction(aprintf("xor %%%%eax, %%%%eax")));
    } else {
             if (hasFpagesInChannel(CHANNEL_OUT))
               addTo(inst1, new CASTAsmInstruction(aprintf("or $2, %%%%eax")));
           }

  if (options & OPTION_FASTCALL)
    {
      addTo(inst1, new CASTAsmInstruction(aprintf("push $0x1b")));
      addTo(inst1, new CASTAsmInstruction(aprintf("push $0f")));
      addTo(inst1, new CASTAsmInstruction(aprintf("mov %%%%esp, %%%%ecx")));
      addTo(inst1, new CASTAsmInstruction(aprintf("sysenter")));
      addTo(inst1, new CASTAsmInstruction(aprintf("mov %%%%ebp, %%%%edx")));
      addTo(inst1, new CASTAsmInstruction(aprintf("0:")));
    } else addTo(inst1, new CASTAsmInstruction(aprintf("int $0x30")));

  addTo(inst1, new CASTAsmInstruction(aprintf("pop %%%%ebp")));

  CASTAsmConstraint *inconst1 = NULL, *outconst1 = NULL;
  long long clobberMask = 0;

  forAllMS(registers[CHANNEL_OUT],
    {
      CMSChunkX *chunk = (CMSChunkX*)item;
      for (int nr = 0; nr < ((chunk->size>4) ? 2 : 1); nr++)
        {
          const char *regName = ((CMSServiceIX*)service)->getShortRegisterName(chunk->clientIndex + nr);
          CASTExpression *cachedInput = chunk->getCachedInput(nr);

          if (cachedInput)
            {
              addTo(inconst1, new CASTAsmConstraint(regName, cachedInput));
              addTo(outconst1, new CASTAsmConstraint(mprintf("=%s", regName), new CASTIdentifier("_dummy")));
              clobberMask |= 1<<(chunk->clientIndex + nr);
            } else warning("Chunk %d (%s): Not initialized in channel %d (subexpr %d)", chunk->chunkID, item->name, CHANNEL_OUT, nr);
        }    
    }
  );
  
  if (!isShortIPC(CHANNEL_OUT))
    {
      CASTExpression *sendDescriptor = new CASTUnaryOp("&", 
        new CASTIdentifier("_par")
      );
        
      addTo(inconst1, new CASTAsmConstraint("a", sendDescriptor));
    }  
    
  addTo(inconst1, service->getThreadidInConstraints(new CASTIdentifier("_client")));
  addTo(outconst1, new CASTAsmConstraint("=a", new CASTIdentifier("_dummy")));
  addTo(outconst1, new CASTAsmConstraint("=S", new CASTIdentifier("_dummy")));
  
  CASTAsmConstraint *clobberconst1 = NULL;
  addTo(clobberconst1, new CASTAsmConstraint("cc"));
  addTo(clobberconst1, new CASTAsmConstraint("memory"));
  addTo(clobberconst1, new CASTAsmConstraint("ecx"));
  for (int i=0;i<numRegs;i++)
    if (!(clobberMask & (1<<i)))
      addTo(clobberconst1, new CASTAsmConstraint(((CMSServiceIX*)service)->getLongRegisterName(i)));
  
  addWithLeadingSpacerTo(result, 
    new CASTAsmStatement(inst1, inconst1, outconst1, clobberconst1, new CASTConstOrVolatileSpecifier("__volatile__"))
  );

  return result;
}
