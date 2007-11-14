#include "arch/x0.h"

CASTStatement *CMSConnectionIXL::buildClientCall(CASTExpression *target, CASTExpression *env) 

{
  int eid = fid + (iid<<FID_SIZE);
  CASTStatement *result = NULL;

  addTo(result, buildClientDopeAssignment(env));

  // EAX UTCB address            EAX Sender Local ID
  // ECX ~                       ECX ~
  // EBP Receive Source          EBP ~
  // ESI Destination             ESI ~
  // EDI msg.w0                  EDI msg.w0
  // EBX msg.w1                  EBX msg.w1
  // EDX msg.w2                  EDX msg.w2

  CASTAsmInstruction *inst1 = NULL;
  addTo(inst1, new CASTAsmInstruction(aprintf("push %%%%ebp")));
  
  if (!isShortIPC(CHANNEL_IN))
    panic("lipc does not support copy");
    
  CASTAsmConstraint *inconst1 = NULL, *outconst1 = NULL, *clobberconst1 = NULL;

  CMSChunkX *regIn = (CMSChunkX*)registers[CHANNEL_IN]->getFirstElement();
  CMSChunkX *regOut = (CMSChunkX*)registers[CHANNEL_OUT]->getFirstElement();

  for (int regNo=0;regNo<2;regNo++)
    {
      const char *regName = NULL;
      
      switch (regNo)
        {
          case 0 : regName = "d";break;
          case 1 : regName = "b";break;
          default: assert(false);
        }  

      CASTAsmConstraint *cIn = NULL, *cOut = NULL;

      if (!regIn->isEndOfList())
        {
          CASTExpression *cachedInput = regIn->getCachedInput(0);
          assert(regIn->size<=4); // no fpages
          
          if (cachedInput)
            cIn = new CASTAsmConstraint(regName, cachedInput);
            else warning("Chunk %d (%s): Not initialized in channel %d", regIn->chunkID, regIn->name, CHANNEL_IN);
            
          regIn = (CMSChunkX*)regIn->next;
        } 
        
      if (!regOut->isEndOfList())
        {
          CASTExpression *cachedOutput = regOut->getCachedOutput(0);
          assert(regOut->size<=4); // no fpages
          assert(cachedOutput);
          
          cOut = new CASTAsmConstraint(mprintf("=%s", regName), cachedOutput);
          regOut = (CMSChunkX*)regOut->next;
        }  
         
      if (cIn && (!cOut))
        cOut = new CASTAsmConstraint(mprintf("=%s", regName), new CASTIdentifier("_dummy"));
        
      if (cIn)
        addTo(inconst1, cIn);
        
      if (cOut)
        addTo(outconst1, cOut);
        else addTo(clobberconst1, new CASTAsmConstraint(mprintf("e%sx", regName)));
    }
  
  addTo(inconst1, new CASTAsmConstraint("D", new CASTIntegerConstant(eid)));
  addTo(inconst1, new CASTAsmConstraint("S", target));

  addTo(clobberconst1, new CASTAsmConstraint("eax"));
  addTo(clobberconst1, new CASTAsmConstraint("ecx"));
  
  addTo(outconst1, new CASTAsmConstraint("=S", new CASTIdentifier("_dummy")));
  addTo(outconst1, new CASTAsmConstraint("=D", new CASTIdentifier("_exception")));

  addTo(inst1, new CASTAsmInstruction(aprintf("mov %%%%gs:(0), %%%%eax")));
  addTo(inst1, new CASTAsmInstruction(aprintf("mov %%%%esi, %%%%ebp")));
  addTo(inst1, new CASTAsmInstruction(aprintf("call \" IDL4_LIPC_ENTRY \"")));
  addTo(inst1, new CASTAsmInstruction(aprintf("pop %%%%ebp")));

  addTo(result, new CASTAsmStatement(inst1, inconst1, outconst1, clobberconst1, new CASTConstOrVolatileSpecifier("__volatile__")));

  addTo(result, new CASTSpacer());
  addTo(result, new CASTIfStatement(
    new CASTBinaryOp("!=",
      env->clone(),
      new CASTIntegerConstant(0)),
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTBinaryOp("->",
          env->clone(),
          new CASTIdentifier("_major")),
        new CASTIdentifier("_exception"))))
  );      

  return result;
}

CASTStatement *CMSConnectionIXL::buildClientLocalVars(CASTIdentifier *key)

{
  CASTStatement *result = NULL;

  addTo(result, new CASTDeclarationStatement(
    msFactory->getMachineWordType()->buildDeclaration(
      new CASTDeclarator(new CASTIdentifier("_dummy"))))
  ); 
  addTo(result, CMSConnectionX::buildClientLocalVarsIndep());
  
  return result;
}

CASTStatement *CMSServiceIXL::buildServerLoop(CASTIdentifier *prefix, CASTExpression *utableRef, CASTExpression *ktableRef, bool useItable, bool hasKernelMessages)

{
  CASTStatement *compound = NULL;

  assert(!useItable);
  
  if (hasKernelMessages)
    panic("local interfaces cannot receive kernel messages");

  CASTIdentifier *fidMask = prefix->clone();
  fidMask->addPostfix("_FID_MASK");

  CASTDeclarator *localIntVars = NULL;
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("dummy")));
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("fnr")));
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("partner")));
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("w0")));
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("w1")));
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("w2")));
  
  addTo(compound,
    new CASTDeclarationStatement(
      msFactory->getMachineWordType()->buildDeclaration(
        localIntVars))
  );
  
  addTo(compound, new CASTSpacer());
  
  CASTStatement *loop = NULL;
  
  CASTExpression *replyAndWaitArgs = NULL;
  addTo(replyAndWaitArgs, new CASTIdentifier("partner"));
  addTo(replyAndWaitArgs, new CASTIdentifier("fnr"));
  addTo(replyAndWaitArgs, new CASTIdentifier("w0"));
  addTo(replyAndWaitArgs, new CASTIdentifier("w1"));
  addTo(replyAndWaitArgs, new CASTIdentifier("w2"));
  addTo(replyAndWaitArgs, new CASTIdentifier("dummy"));
  
  addTo(loop, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("idl4_local_reply_and_wait"),
      replyAndWaitArgs))
  );

  CASTExpression *processRequestArgs = NULL;
  addTo(processRequestArgs, utableRef);
  addTo(processRequestArgs, new CASTBinaryOp("&", new CASTIdentifier("fnr"), fidMask));
  addTo(processRequestArgs, new CASTIdentifier("partner"));
  addTo(processRequestArgs, new CASTIdentifier("w0"));
  addTo(processRequestArgs, new CASTIdentifier("w1"));
  addTo(processRequestArgs, new CASTIdentifier("w2"));
  addTo(processRequestArgs, new CASTIdentifier("dummy"));

  addTo(loop, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("idl4_local_process_request"),
      processRequestArgs))
  );

  addTo(compound,
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTIdentifier("w0"),
        new CASTBinaryOp("=",
          new CASTIdentifier("w1"),
          new CASTBinaryOp("=",
            new CASTIdentifier("w2"),
            new CASTIntegerConstant(0)))))
  );
  addTo(compound,
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTIdentifier("partner"),
        new CASTHexConstant(0xFFFFFFFF)))
  );
  addTo(compound,
    new CASTWhileStatement(
      new CASTIntegerConstant(1),
      new CASTCompoundStatement(loop))
  );
  
  return compound;
}

CASTStatement *CMSConnectionIXL::buildServerDeclarations(CASTIdentifier *key)

{
  CASTIdentifier *wrapperIdentifier = key->clone();
  wrapperIdentifier->addPrefix("service_");
  
  CBEType *mwType = msFactory->getMachineWordType();
  CASTDeclaration *wrapperParams = NULL;
  addTo(wrapperParams, new CASTDeclaration(
    new CASTTypeSpecifier(new CASTIdentifier("l4_threadid_t")),
    new CASTDeclarator(new CASTIdentifier("_caller"))));
  addTo(wrapperParams, mwType->buildDeclaration(
    new CASTDeclarator(new CASTIdentifier("w0"))));
  addTo(wrapperParams, mwType->buildDeclaration(
    new CASTDeclarator(new CASTIdentifier("w1"))));
  addTo(wrapperParams, mwType->buildDeclaration(
    new CASTDeclarator(new CASTIdentifier("_stack_top"))));
    
  CBEType *voidType = getType(aoiRoot->lookupSymbol("#void", SYM_TYPE));
  return new CASTDeclarationStatement(
    voidType->buildDeclaration(
      new CASTDeclarator(
        wrapperIdentifier,
        wrapperParams,
        NULL, NULL, NULL, 
        new CASTAttributeSpecifier(
          new CASTIdentifier("regparm"),
          new CASTIntegerConstant(3)
        )))
  );
}

CASTBase *CMSConnectionIXL::buildServerWrapper(CASTIdentifier *key, CASTCompoundStatement *compound)

{
  CBEType *voidType = getType(aoiRoot->lookupSymbol("#void", SYM_TYPE));

  CBEType *mwType = msFactory->getMachineWordType();
  CASTDeclaration *wrapperParams = NULL;
  addTo(wrapperParams, new CASTDeclaration(
    new CASTTypeSpecifier(new CASTIdentifier("l4_threadid_t")),
    new CASTDeclarator(new CASTIdentifier("_caller"))));
  addTo(wrapperParams, mwType->buildDeclaration(
    new CASTDeclarator(new CASTIdentifier("w0"))));
  addTo(wrapperParams, mwType->buildDeclaration(
    new CASTDeclarator(new CASTIdentifier("w1"))));
  addTo(wrapperParams, mwType->buildDeclaration(
    new CASTDeclarator(new CASTIdentifier("_stack_top"))));
  
  CASTIdentifier *wrapperIdentifier = key->clone();
  wrapperIdentifier->addPrefix("service_");
  
  return voidType->buildDeclaration(
    new CASTDeclarator(wrapperIdentifier, wrapperParams), compound);
}

CASTExpression *CMSConnectionIXL::buildFCCDataSourceExpr(int channel, ChunkID chunkID)

{
  /* On the server, register targets are actually in the buffer */
  
  if (channel!=CHANNEL_IN)
    {
      int regNo = 0;
      forAllMS(registers[channel],
        {
          CMSChunkX *chunk = (CMSChunkX*)item;
          if (chunk->chunkID == chunkID)
            return new CASTIdentifier(mprintf("w%d", regNo));
            else regNo++;
        }
      );        
    }    

  panic("Cannot build FCC data inbuf expression (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTExpression *CMSConnectionIXL::buildFCCDataTargetExpr(int channel, ChunkID chunkID)

{
  /* On the server, register targets are actually in the buffer */
  
  if (channel==CHANNEL_IN)
    {
      int regNo = 0;
      forAllMS(registers[channel],
        {
          CMSChunkX *chunk = (CMSChunkX*)item;
          if (chunk->chunkID == chunkID)
            return new CASTIdentifier(mprintf("w%d", regNo));
            else regNo++;
        }
      );        
    }    

  panic("Cannot build FCC data target expression (chunk %d in channel %d)", chunkID, channel);
  return NULL;
}

CASTExpression *CMSConnectionIXL::buildServerCallerID()

{
  return new CASTIdentifier("_caller");
}

CASTStatement *CMSConnectionIXL::buildServerBackjump()

{
  CASTStatement *result = NULL;
  CASTAsmInstruction *jumpInstructions = NULL;

  addTo(jumpInstructions, new CASTAsmInstruction(aprintf("leal -4+%%0, %%%%esp")));
  addTo(jumpInstructions, new CASTAsmInstruction(aprintf("ret")));
 
  CASTAsmConstraint *inConstraints = NULL;
  addTo(inConstraints, new CASTAsmConstraint("m",
    new CASTIdentifier("_stack_top")));
  addTo(inConstraints, new CASTAsmConstraint("S", 
    new CASTFunctionOp(
      new CASTIdentifier("idl4_raw_threadid"),
      new CASTIdentifier("_caller")))
  );      
  
  int regNo = 0;
  forAllMS(registers[CHANNEL_OUT],
    {
      addTo(inConstraints, new CASTAsmConstraint((regNo==0) ? "d" : "b",
        new CASTIdentifier(mprintf("w%d", regNo))));
      regNo++;
    });  

  addTo(inConstraints, new CASTAsmConstraint("D", new CASTIntegerConstant(0)));

  addTo(result, new CASTAsmStatement(jumpInstructions, inConstraints, NULL, NULL, new CASTConstOrVolatileSpecifier("__volatile__")));
  addTo(result, new CASTWhileStatement(new CASTIntegerConstant(1), NULL));
  
  return result;
}

CASTExpression *CMSConnectionIXL::buildStackTopExpression()

{
  return new CASTIdentifier("_stack_top");
}

CASTExpression *CMSConnectionIXL::buildCallerExpression()

{
  return new CASTIdentifier("_caller");
}
