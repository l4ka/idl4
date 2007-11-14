#include "arch/v4.h"

CASTExpression *CMSConnectionI4::buildLabelExpr()

{
  return new CASTBinaryOp(">>",
    new CASTIdentifier("_result"),
    new CASTIntegerConstant(16)
  );
}

CASTExpression *CMSConnectionI4::buildClientCallSucceeded()

{
  return new CASTBinaryOp("==",
    new CASTBinaryOp("&",
      new CASTIdentifier("_result"),
      new CASTHexConstant((options&OPTION_ONEWAY) ? 0x8000 : 0xFFFF8000, true)),
    new CASTIntegerConstant(0)
  );
}

CASTStatement *CMSConnectionI4::buildClientLocalVars(CASTIdentifier *key)

{
  CASTDeclarationStatement *result = buildClientStandardVars();

  addTo(result, new CASTDeclarationStatement(
    msFactory->getMachineWordType()->buildDeclaration(
        new CASTDeclarator(
          new CASTIdentifier("_result"))))
  );

  addTo(result, new CASTDeclarationStatement(
    msFactory->getMachineWordType()->buildDeclaration(
      new CASTDeclarator(
        new CASTIdentifier("_dummy"))))
  );

  addTo(result, new CASTDeclarationStatement(
    new CASTDeclaration(
      buildClientMsgBuf(),
      new CASTDeclarator(
        new CASTIdentifier("_pack"),
        new CASTPointer(),
        NULL,
        new CASTUnaryOp("(union _buf*)",
          new CASTFunctionOp(
            new CASTIdentifier("MyUTCB"))))))
  );

  return result;
}

CASTExpression *CMSConnectionI4::buildSourceBufferRvalue(int channel)

{
  CASTIdentifier *bufferName;
  
  if (channel==CHANNEL_IN) 
    bufferName = new CASTIdentifier("_pack");
    else bufferName = new CASTIdentifier("_par");
    
  return new CASTBinaryOp("->",
    bufferName,
    buildChannelIdentifier(channel));
}

CASTExpression *CMSConnectionI4::buildTargetBufferRvalue(int channel)

{
  CASTIdentifier *bufferName;
  
  if (channel==CHANNEL_IN) 
    bufferName = new CASTIdentifier("_par");
    else bufferName = new CASTIdentifier("_pack");
    
  return new CASTBinaryOp("->",
    bufferName,
    buildChannelIdentifier(channel));
}

CASTStatement *CMSConnectionI4::buildClientResultAssignment(CASTExpression *environment, CASTExpression *defaultValue)

{
  CASTStatement *userCodeAssignment = NULL;
  
  if (!(options&OPTION_ONEWAY))
    userCodeAssignment = new CASTExpressionStatement(
    new CASTBinaryOp("=",
      new CASTUnaryOp("*",
        new CASTUnaryOp("(unsigned*)",
          environment->clone())),
      defaultValue)
  );

  return new CASTCompoundStatement(
    new CASTIfStatement(
      new CASTBinaryOp("!=",
        new CASTBinaryOp("&",
          new CASTIdentifier("_result"),
          new CASTHexConstant(0x00008000)),
      new CASTIntegerConstant(0)),
      new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTUnaryOp("*",
            new CASTUnaryOp("(unsigned*)",
              environment->clone())),
          new CASTBinaryOp("+", 
            new CASTIdentifier("CORBA_SYSTEM_EXCEPTION"),
            new CASTBinaryOp("<<",
              new CASTFunctionOp(new CASTIdentifier("ErrorCode")),
              new CASTIntegerConstant(8))))),
      userCodeAssignment)
  );
}

CASTAttributeSpecifier *CMSConnectionI4::buildWrapperAttributes()

{
  CASTAttributeSpecifier *attrs = NULL;
  
  addTo(attrs, new CASTAttributeSpecifier(
    new CASTIdentifier("regparm"),
    new CASTIntegerConstant(2))
  );
  addTo(attrs, new CASTAttributeSpecifier(
    new CASTIdentifier("noreturn"))
  );
  
  return attrs;
}

CASTDeclaration *CMSConnectionI4::buildWrapperParams(CASTIdentifier *key)

{
  CASTDeclaration *wrapperParams = NULL;
  
  addTo(wrapperParams, msFactory->getThreadIDType()->buildDeclaration(
    new CASTDeclarator(new CASTIdentifier("_caller")))
  );
  addTo(wrapperParams, new CASTDeclaration(
    new CASTTypeSpecifier(buildServerParamID(key)),
    new CASTDeclarator(new CASTIdentifier("_par"), new CASTPointer(1)))
  );  
  addTo(wrapperParams, msFactory->getMachineWordType()->buildDeclaration(
    new CASTDeclarator(new CASTIdentifier("_stack_top")))
  );
  
  return wrapperParams;
}

CBEType *CMSConnectionI4::getWrapperReturnType()

{
  return getType(aoiRoot->lookupSymbol("#void", SYM_TYPE));
}

CASTStatement *CMSConnectionI4::buildServerAbort()

{ 
  CASTStatement *result = NULL;
  
  CASTAsmInstruction *noResponseInstructions = NULL;
  addTo(noResponseInstructions, new CASTAsmInstruction(aprintf("lea -4+%%0, %%%%esp")));
  addTo(noResponseInstructions, new CASTAsmInstruction(aprintf("ret")));

  CASTAsmConstraint *noResponseConstraints = NULL;
  addTo(noResponseConstraints, new CASTAsmConstraint("m", new CASTIdentifier("_stack_top")));
  addTo(noResponseConstraints, new CASTAsmConstraint("d", new CASTIntegerConstant(0)));
  addTo(noResponseConstraints, new CASTAsmConstraint("c", new CASTIntegerConstant(0)));

  addTo(result, new CASTAsmStatement(
    noResponseInstructions, 
    noResponseConstraints, 
    NULL, NULL, 
    new CASTConstOrVolatileSpecifier("__volatile__"))
  );
  
  addWithLeadingSpacerTo(result, new CASTWhileStatement(
    new CASTIntegerConstant(1))
  );

  addTo(result, new CASTExpressionStatement(
    new CASTUnaryOp("(void)", 
      new CASTIdentifier("_par")))
  );
  
  return result;
}

CASTStatement *CMSConnectionI4::buildServerBackjump(int channel, CASTExpression *environment)

{
  CASTStatement *result = NULL;

  addTo(result, buildMemMsgSetup(CHANNEL_OUT));

  CASTAsmInstruction *jumpInstructions = NULL;
  addTo(jumpInstructions, new CASTAsmInstruction(aprintf("lea -4+%%0, %%%%esp")));
  addTo(jumpInstructions, new CASTAsmInstruction(aprintf("ret")));
 
  CASTAsmConstraint *inConstraints = NULL;
  addTo(inConstraints, new CASTAsmConstraint("m", new CASTIdentifier("_stack_top")));
  addTo(inConstraints, new CASTAsmConstraint("d", 
    new CASTIdentifier("_caller"))
  );      
  
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
  
  addTo(inConstraints, new CASTAsmConstraint("a", mr0));
  addTo(inConstraints, new CASTAsmConstraint("c",
    new CASTIntegerConstant(untypedWords + specialWords))
  );

 addTo(result, new CASTAsmStatement(
   jumpInstructions,
   inConstraints,
   NULL, NULL,
   new CASTConstOrVolatileSpecifier("__volatile__"))
 );
 
 return result;
}

CASTStatement *CMSConnectionI4::buildServerReplyDeclarations(CASTIdentifier *key)

{
  CBEType *mwType = msFactory->getMachineWordType();

  CASTStatement *result = NULL;
  
  addTo(result, new CASTDeclarationStatement(
    mwType->buildDeclaration(
      new CASTDeclarator(new CASTIdentifier("dummy"))))
  );

  addTo(result, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTAggregateSpecifier("struct", 
        new CASTIdentifier("_reply_buffer"), 
        new CASTDeclarationStatement(
          new CASTDeclaration(
            new CASTAggregateSpecifier("struct",
              ANONYMOUS,
              buildMessageMembers(CHANNEL_OUT)),
            new CASTDeclarator(
              buildChannelIdentifier(CHANNEL_OUT))))),
      new CASTDeclarator(
        new CASTIdentifier("_par"),
        new CASTPointer(1),
        NULL,
        new CASTCastOp(
          new CASTDeclaration(
            new CASTAggregateSpecifier("struct",
              new CASTIdentifier("_reply_buffer")),
            new CASTDeclarator(
              (CASTDeclarator*)NULL,
              new CASTPointer())),
          new CASTFunctionOp(
            new CASTIdentifier("MyUTCB")),
          CASTStandardCast))))
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

CASTStatement *CMSConnectionI4::buildServerReply()

{
  CASTStatement *result = NULL;

  CASTAsmConstraint *inconst = NULL;
  addTo(inconst, new CASTAsmConstraint("a", new CASTIdentifier("_client")));
  addTo(inconst, new CASTAsmConstraint("c", new CASTHexConstant(0x04000000)));
  addTo(inconst, new CASTAsmConstraint("d", new CASTIntegerConstant(0)));
  addTo(inconst, new CASTAsmConstraint("S", buildMsgTag(CHANNEL_OUT)));
  addTo(inconst, new CASTAsmConstraint("D", new CASTIdentifier("_par")));
  
  CASTAsmConstraint *outconst = NULL;
  addTo(outconst, new CASTAsmConstraint("=a", new CASTIdentifier("dummy")));
  addTo(outconst, new CASTAsmConstraint("=c", new CASTIdentifier("dummy")));
  addTo(outconst, new CASTAsmConstraint("=d", new CASTIdentifier("dummy")));
  addTo(outconst, new CASTAsmConstraint("=S", new CASTIdentifier("dummy")));
  addTo(outconst, new CASTAsmConstraint("=D", new CASTIdentifier("dummy")));
  
  CASTAsmConstraint *clobberconst = NULL;
  addTo(clobberconst, new CASTAsmConstraint("ebx"));
  addTo(clobberconst, new CASTAsmConstraint("memory"));
  addTo(clobberconst, new CASTAsmConstraint("cc"));

  CASTAsmInstruction *inst = NULL;
  addTo(inst, new CASTAsmInstruction(aprintf("push %%%%ebp")));
  if (options & OPTION_FASTCALL) {
    addTo(inst, new CASTAsmInstruction(aprintf("movl %%%%esp, %%%%ebp")));
    addTo(inst, new CASTAsmInstruction(aprintf("lea 0f, %%%%ebx")));
    addTo(inst, new CASTAsmInstruction(aprintf("movl %%%%esi, 0(%%%%edi)")));
    addTo(inst, new CASTAsmInstruction(aprintf("sysenter")));
    addTo(inst, new CASTAsmInstruction(aprintf("0:")));
  } else {
    addTo(inst, new CASTAsmInstruction(aprintf("call __L4_Ipc")));
  }
  addTo(inst, new CASTAsmInstruction(aprintf("pop %%%%ebp")));

  if (!mem_fixed[CHANNEL_OUT]->isEmpty() || !mem_variable[CHANNEL_OUT]->isEmpty())
    addTo(result, buildMemMsgSetup(CHANNEL_OUT));

  addTo(result, new CASTAsmStatement(inst, inconst, outconst, clobberconst, new CASTConstOrVolatileSpecifier("__volatile__")));
  
  return result;
}
