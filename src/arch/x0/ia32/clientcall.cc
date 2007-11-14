#include "arch/x0.h"

CASTStatement *CMSConnectionIX::buildClientCall(CASTExpression *target, CASTExpression *env) 

{
  CASTStatement *result = NULL;

  addTo(result, buildClientDopeAssignment(env));

  // EAX Send descriptor         EAX Msg dope
  // ECX Timeout                 ECX ~
  // EBP Recv descriptor         EBP ~
  // ESI Destination             ESI Source
  // EDI msg.w0                  EDI msg.w0
  // EBX msg.w1                  EBX msg.w1
  // EDX msg.w2                  EDX msg.w2

  // Sysenter:
  //    PUSH timeout
  //    PUSH recv descriptor
  //    PUSH linear_space_exec (0x1b)
  //    PUSH offset R
  //    MOV  esp, ecx
  //    SYSENTER
  //    mov  ebp, edx
  // R:

  CASTAsmInstruction *inst1 = NULL;
  addTo(inst1, new CASTAsmInstruction(aprintf("push %%%%ebp")));

  if (!isShortIPC(CHANNEL_IN) && hasOutFpages() && !hasLongOutIPC())
    {
      addTo(inst1, new CASTAsmInstruction(aprintf("mov 0(%%%%eax), %%%%ebp")));
      addTo(inst1, new CASTAsmInstruction(aprintf("and $0xFFFFFFFC, %%%%ebp")));
      addTo(inst1, new CASTAsmInstruction(aprintf("or $2, %%%%ebp")));
    }
    
  // Receive descriptor
  
  if (!(options & OPTION_FASTCALL))
    {
      if (hasLongOutIPC() || hasOutFpages())
        addTo(inst1, new CASTAsmInstruction(aprintf("mov %%%%eax, %%%%ebp")));
        else {
               if (options&OPTION_ONEWAY)
                 addTo(inst1, new CASTAsmInstruction(aprintf("mov $-1, %%%%ebp")));
                 else addTo(inst1, new CASTAsmInstruction(aprintf("xor %%%%ebp, %%%%ebp")));
             }
    } else {    
             addTo(inst1, new CASTAsmInstruction(aprintf("push %%%%ecx")));
             if (hasLongOutIPC() || hasOutFpages())
               addTo(inst1, new CASTAsmInstruction(aprintf("push %%%%eax")));
               else {
                      if (options&OPTION_ONEWAY)
                        addTo(inst1, new CASTAsmInstruction(aprintf("push $-1")));
                        else addTo(inst1, new CASTAsmInstruction(aprintf("push $0")));
                    }
           }
           
  if (isShortIPC(CHANNEL_IN))
    {
      if (hasFpagesInChannel(CHANNEL_IN))
        addTo(inst1, new CASTAsmInstruction(aprintf("mov $2, %%%%eax")));
        else addTo(inst1, new CASTAsmInstruction(aprintf("xor %%%%eax, %%%%eax")));
    } else {
             if (hasFpagesInChannel(CHANNEL_IN))
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
    } else {
             addTo(inst1, new CASTAsmInstruction(aprintf("int $0x30")));
           }  

  CASTAsmConstraint *inconst1 = NULL;

  forAllMS(registers[CHANNEL_IN],
    {
      CMSChunkX *chunk = (CMSChunkX*)item;
      for (int nr = 0; nr < ((chunk->size>4) ? 2 : 1); nr++)
        {
          const char *regName = ((CMSServiceIX*)service)->getShortRegisterName(chunk->clientIndex + nr);
          CASTExpression *cachedInput = chunk->getCachedInput(nr);

          if (cachedInput)
            addTo(inconst1, new CASTAsmConstraint(regName, cachedInput));
            else warning("Chunk %d (%s): Not initialized in channel %d (subexpr %d)", chunk->chunkID, item->name, CHANNEL_IN, nr);
        }    
    }
  );
  
  if (!isShortIPC(CHANNEL_IN) || hasLongOutIPC())
    {
      CASTExpression *sendDescriptor = new CASTUnaryOp("&", 
        new CASTIdentifier("_pack")
      );
        
      addTo(inconst1, new CASTAsmConstraint("a", sendDescriptor));
    }  
    
  addTo(inconst1, service->getThreadidInConstraints(target));
  
  /* If we want to receive exactly one fpage, we need to load the receive
     window into EBP. As this cannot be done directly, we use EAX instead */

  if (!hasLongOutIPC() && hasOutFpages() && isShortIPC(CHANNEL_IN))
    addTo(inconst1, new CASTAsmConstraint("a", 
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
          new CASTIntegerConstant(2))))
    );
      
  addTo(inconst1, new CASTAsmConstraint("c", 
    new CASTConditionalExpression(
      new CASTBrackets(
        new CASTBinaryOp("==", 
          env, 
          new CASTIntegerConstant(0))),
      new CASTIdentifier("L4_IPC_NEVER"),
      new CASTBinaryOp("->", env, new CASTIdentifier("_timeout"))))
  );    

  CASTAsmConstraint *outconst1 = NULL;
  CASTAsmConstraint *clobberconst1 = NULL;
  addTo(clobberconst1, new CASTAsmConstraint("cc"));
  addTo(clobberconst1, new CASTAsmConstraint("memory"));
  
  addTo(result, new CASTAsmStatement(inst1, inconst1, outconst1, clobberconst1, new CASTConstOrVolatileSpecifier("__volatile__")));
  addTo(result, new CASTSpacer());

// ****

  CASTAsmInstruction *inst2 = NULL;
  addTo(inst2, new CASTAsmInstruction(aprintf("pop %%%%ebp")));

  CASTAsmConstraint *outconst2 = NULL;

  long long clobberMask = 0;
  forAllMS(registers[CHANNEL_OUT],
    {
      CMSChunkX *chunk = (CMSChunkX*)item;
      for (int nr = 0; nr < ((chunk->size>4) ? 2 : 1); nr++)
        {
          char *regName = mprintf("=%s", ((CMSServiceIX*)service)->getShortRegisterName(chunk->clientIndex + nr));
          clobberMask |= 1<<(chunk->clientIndex + nr);

          CASTExpression *cachedOutput = chunk->getCachedOutput(nr);
          if (cachedOutput)
            addTo(outconst2, new CASTAsmConstraint(regName, cachedOutput));
        }
    }
  );

  CASTAsmConstraint *clobberconst2 = NULL;
  for (int i=0;i<numRegs;i++)
    if (!(clobberMask & (1<<i)))
      addTo(clobberconst2, new CASTAsmConstraint(((CMSServiceIX*)service)->getLongRegisterName(i)));
  addTo(clobberconst2, new CASTAsmConstraint("ecx"));
  addTo(clobberconst2, new CASTAsmConstraint("esi"));
  
  addTo(outconst2, new CASTAsmConstraint("=a", new CASTIdentifier("_msgDope")));

  addTo(result, new CASTAsmStatement(inst2, NULL, outconst2, clobberconst2, new CASTConstOrVolatileSpecifier("__volatile__")));

  if (!vars[CHANNEL_OUT]->isEmpty())
    {  
      addTo(result, new CASTSpacer());
      addTo(result, 
        new CASTExpressionStatement(
          new CASTBinaryOp("=",
            new CASTIdentifier("_varidx"),
            new CASTIntegerConstant(0)))
      );
    }  

  return result;
}

CASTStatement *CMSConnectionIX::buildClientLocalVars(CASTIdentifier *key)

{
  CASTStatement *result = NULL;

  addTo(result, CMSConnectionX::buildClientLocalVarsIndep());

  addTo(result, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTTypeSpecifier("idl4_msgdope_t"),
        new CASTDeclarator(
          new CASTIdentifier("_msgDope"))))
  );
  
  return result;
}

