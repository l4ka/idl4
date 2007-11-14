#include "arch/x0.h"

CASTStatement *CMSConnectionX::buildClientDopeAssignment(CASTExpression *env)

{
  CASTStatement *result = NULL;

  if (!isShortIPC(CHANNEL_IN) || hasLongOutIPC())
    {
      CASTExpression *longBuffer;

      if (isShortIPC(CHANNEL_IN))
        longBuffer = buildTargetBufferRvalue(CHANNEL_OUT);
        else longBuffer = buildSourceBufferRvalue(CHANNEL_IN);
    
      /* only if strings are present, or we have a long out */
      if ((!strings[CHANNEL_IN]->isEmpty()) || hasLongOutIPC() || hasOutFpages())
        {
          if (hasOutFpages())
            {
              addTo(result, new CASTExpressionStatement(
                new CASTBinaryOp("=",
                  new CASTBinaryOp(".",
                    longBuffer->clone(),
                    new CASTIdentifier("_rcv_window")),
                  new CASTBinaryOp("->",
                    env->clone(),
                    new CASTIdentifier("_rcv_window"))))
              );    
            } else {
                     addTo(result, new CASTExpressionStatement(
                       new CASTBinaryOp("=",
                         new CASTBinaryOp(".",
                           longBuffer->clone(),
                           new CASTIdentifier("_rcv_window")),
                         new CASTIdentifier("idl4_nilpage")))
                     );
                   }

          if ((!strings[CHANNEL_IN]->isEmpty()) || hasLongOutIPC())
            addTo(result, new CASTExpressionStatement(
              new CASTBinaryOp("=",
                new CASTBinaryOp(".",
                  longBuffer->clone(),
                  new CASTIdentifier("_size_dope")),
                buildClientSizeDope()))
            );
        }  

      if (!isShortIPC(CHANNEL_IN))
        addTo(result, new CASTExpressionStatement(
          new CASTBinaryOp("=",
            new CASTBinaryOp(".",
              longBuffer->clone(),
              new CASTIdentifier("_send_dope")),
            buildSendDope(CHANNEL_IN)))
        );
    }  

  /* Some elements of the message may require initialization; for example, the
     map delimiters must be set to zero */

  forAllMS(dwords[CHANNEL_IN],
    {
      CMSChunkX *chunk = (CMSChunkX*)item;
      if (chunk->flags & CHUNK_INIT_ZERO)
        {
          for (int i=0;i<2;i++)
            addTo(result, new CASTExpressionStatement(
              new CASTBinaryOp("=",
                new CASTIndexOp(
                  new CASTBinaryOp(".",
                    buildSourceBufferRvalue(CHANNEL_IN),
                    new CASTIdentifier("__mapPad")),
                  new CASTIntegerConstant(i)),  
                new CASTIntegerConstant(0)))
            );      
        }
    }
  );

  if (result)
    addTo(result, new CASTSpacer());

  return result;
}

const char *CMSConnectionX::getIPCBindingName(bool sendOnly)

{ 
  if (sendOnly)
    return "l4_ipc_send";
    else return "l4_ipc_call";
}

CASTStatement *CMSConnectionX::buildClientCall(CASTExpression *target, CASTExpression *env) 

{
  CASTStatement *result = NULL;

  addTo(result, buildClientDopeAssignment(env));
  
  CASTExpression *args = NULL;
  addTo(args, target);
  if (isShortIPC(CHANNEL_IN))
    {
      if (hasFpagesInChannel(CHANNEL_IN))
        addTo(args, new CASTIdentifier("IDL4_IPC_REG_FPAGE"));
        else addTo(args, new CASTIdentifier("IDL4_IPC_REG_MSG"));
    } else {
             CASTExpression *sendDope = new CASTUnaryOp("&", new CASTIdentifier("_pack"));
             if (hasFpagesInChannel(CHANNEL_IN))
               sendDope = new CASTCastOp(
                 new CASTDeclaration(
                   new CASTTypeSpecifier("void"),
                   new CASTDeclarator((CASTIdentifier*)NULL, new CASTPointer())),
                 new CASTBinaryOp("|",
                   new CASTIntegerConstant(2),
                   new CASTCastOp(
                     msFactory->getMachineWordType()->buildDeclaration(NULL),
                     sendDope,
                     CASTStandardCast)),
                 CASTStandardCast
               );
               
             addTo(args, sendDope);
           }  

  int currentReg = 0;
  forAllMS(registers[CHANNEL_IN],
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

  CASTStatement *reassignment = NULL;
  if (!(options&OPTION_ONEWAY))
    {
      if (!hasLongOutIPC())
        {
          if (hasOutFpages())
            {
              addTo(args, new CASTCastOp(
                new CASTDeclaration(
                  new CASTTypeSpecifier("void"),
                  new CASTDeclarator((CASTIdentifier*)NULL, new CASTPointer())),
                new CASTConditionalExpression(
                  new CASTBrackets(
                    new CASTBinaryOp("==",
                      env->clone(),
                      new CASTIntegerConstant(0))),
                  new CASTBinaryOp("|",
                    new CASTBinaryOp("<<",
                      new CASTIdentifier("L4_WHOLE_ADDRESS_SPACE"),
                      new CASTIntegerConstant(2)),    
                    new CASTIntegerConstant(2)),  
                  new CASTBinaryOp("|",
                    new CASTBrackets(
                      new CASTBinaryOp("&", 
                        new CASTFunctionOp(
                          new CASTIdentifier("idl4_raw_fpage"),
                            new CASTBinaryOp("->",
                              env->clone(),
                              new CASTIdentifier("_rcv_window"))),
                        new CASTHexConstant(0xFFFFFFFC, true))),
                    new CASTIntegerConstant(2))), CASTStandardCast)
              );
            } else addTo(args, new CASTIdentifier("IDL4_IPC_REG_MSG"));
        } else addTo(args, new CASTUnaryOp("&", new CASTIdentifier("_pack")));

      currentReg = 0;
      forAllMS(registers[CHANNEL_OUT],
        {
          CMSChunkX *chunk = (CMSChunkX*)item;

          assert(currentReg<=chunk->clientIndex);
          while (currentReg<chunk->clientIndex)
            {
              addTo(args, new CASTUnaryOp("(unsigned*)&", new CASTIdentifier("_dummy")));
              currentReg++;
            }
      
          for (int nr = 0; nr < ((chunk->size>4) ? 2 : 1); nr++)
            {
              CASTExpression *cachedOutput = chunk->getCachedOutput(nr);
              
              if (cachedOutput)
                {
                  if ((chunk->size - 4*nr)<4)
                    {
                      /* Special case for register values shorter than a machine word.
                         The call binding always assigns entire machine words, so
                         the surrounding values in memory would be overwritten. 
                         To avoid this, we allocate buffer variables in the stub
                         and assign indirectly */
            
                      addTo(args, new CASTUnaryOp("(unsigned*)&", new CASTIdentifier(mprintf("_regbuf%d", currentReg))));
              
                      const char *castType;
                      switch (chunk->type->getAlignment())
                        {
                          case 1 : castType = "char";break;
                          case 2 : castType = "short";break;
                          default: castType = "int";break;
                        }
                  
                      addTo(reassignment, new CASTExpressionStatement(
                        new CASTBinaryOp("=",
                          new CASTUnaryOp(mprintf("*(%s*)&", castType), cachedOutput),
                          new CASTIdentifier(mprintf("_regbuf%d", currentReg))))
                      );    
                    } else addTo(args, new CASTUnaryOp("(unsigned*)&", cachedOutput));
                } else warning("Chunk %d (%s): Not initialized in channel %d (subexpr %d)", chunk->chunkID, item->name, CHANNEL_OUT, nr);
                
              currentReg++;  
            }    
        }
      );

      assert(currentReg<=numRegs);
      while (currentReg<numRegs)
        addTo(args, new CASTUnaryOp("(unsigned*)&", new CASTIdentifier("_dummy")));
    }  
    
  addTo(args, new CASTConditionalExpression(
    new CASTBrackets(
      new CASTBinaryOp("==",
        env->clone(),
        new CASTIntegerConstant(0))),
    new CASTIdentifier("L4_IPC_NEVER"),
    new CASTBinaryOp("->",
      env,
      new CASTIdentifier("_timeout"))));
  addTo(args, new CASTUnaryOp("&", new CASTIdentifier("_msgDope")));
  
  const char *callName;
  if (options&OPTION_ONEWAY)
    callName = getIPCBindingName(true);
    else callName = getIPCBindingName(false);
  
  addTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier(callName),
      args))
  );
  addTo(result, reassignment);

  if (!vars[CHANNEL_OUT]->isEmpty())
    addWithLeadingSpacerTo(result, 
      new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTIdentifier("_varidx"),
          new CASTIntegerConstant(0)))
    );
    
  return result;
}

CASTStatement *CMSConnectionX::buildClientLocalVars(CASTIdentifier *key)

{
  CASTStatement *result = NULL;
  CASTDeclarator *requiredInts = NULL;
  int regsUsed = 0;
 
  forAllMS(registers[CHANNEL_OUT], 
    {
      CMSChunkX *chunk = (CMSChunkX*)item;
      
      if (chunk->size<4)
        addTo(requiredInts, new CASTDeclarator(new CASTIdentifier(mprintf("_regbuf%d", chunk->clientIndex))));
        
      regsUsed += (chunk->size>4) ? 2 : 1;
    }
  );    

  if ((regsUsed<numRegs) && (!(options&OPTION_ONEWAY)))
    addTo(requiredInts, new CASTDeclarator(new CASTIdentifier("_dummy")));

  if (requiredInts)  
    addTo(result, new CASTDeclarationStatement(
      msFactory->getMachineWordType()->buildDeclaration(
        requiredInts))
    ); 
      
  addTo(result, CMSConnectionX::buildClientLocalVarsIndep());

  addTo(result, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTTypeSpecifier("l4_msgdope_t"),
        new CASTDeclarator(
          new CASTIdentifier("_msgDope"))))
  );

  return result;
}
