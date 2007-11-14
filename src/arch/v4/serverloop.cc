#include "arch/v4.h"

CASTStatement *CMSService4::buildServerDeclarations(CASTIdentifier *prefix, int vtableSize, int itableSize, int ktableSize)

{
  CASTIdentifier *fidMask = prefix->clone();
  CASTIdentifier *iidMask = prefix->clone();
  CASTIdentifier *kidMask = prefix->clone();
  CASTIdentifier *msgbufSize = prefix->clone();
  CASTIdentifier *strbufSize = prefix->clone();
  
  msgbufSize->addPostfix("_MSGBUF_SIZE");
  strbufSize->addPostfix("_STRBUF_SIZE");
  fidMask->addPostfix("_FID_MASK");
  iidMask->addPostfix("_IID_MASK");
  kidMask->addPostfix("_KID_MASK");

  CASTStatement *result = NULL;
  addTo(result, new CASTPreprocDefine(msgbufSize, new CASTIntegerConstant(numDwords)));
  addTo(result, new CASTPreprocDefine(strbufSize, new CASTIntegerConstant(numStrings)));
  addTo(result, new CASTPreprocDefine(fidMask, new CASTHexConstant(vtableSize-1)));
  if (itableSize>0)
    addTo(result, new CASTPreprocDefine(iidMask, new CASTHexConstant(itableSize-1)));
  if (ktableSize>0)
    addTo(result, new CASTPreprocDefine(kidMask, new CASTHexConstant(ktableSize-1)));
    
  return result;
}

CASTStatement *CMSService4::buildServerLoop(CASTIdentifier *prefix, CASTExpression *utableRef, CASTExpression *ktableRef, bool useItable, bool hasKernelMessages)

{
  CASTIdentifier *fidMask = prefix->clone();
  CASTIdentifier *iidMask = prefix->clone();
  CASTIdentifier *kidMask = prefix->clone();
  CASTIdentifier *strbufSize = prefix->clone();

  fidMask->addPostfix("_FID_MASK");
  iidMask->addPostfix("_IID_MASK");
  kidMask->addPostfix("_KID_MASK");
  strbufSize->addPostfix("_STRBUF_SIZE");

  CASTStatement *compound = NULL;

  addTo(compound,
    new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTTypeSpecifier(
          new CASTIdentifier("L4_ThreadId_t")),
        new CASTDeclarator(
          new CASTIdentifier("partner"))))
  );
  addTo(compound,
    new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTTypeSpecifier(
          new CASTIdentifier("L4_MsgTag_t")),
        new CASTDeclarator(
          new CASTIdentifier("msgtag"))))
  );
  addTo(compound,
    new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTTypeSpecifier(new CASTIdentifier("idl4_msgbuf_t")),
        new CASTDeclarator(new CASTIdentifier("msgbuf"))))
  );
  addTo(compound,
    new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTTypeSpecifier(
          new CASTIdentifier("long")),
        new CASTDeclarator(
          new CASTIdentifier("cnt"))))
  );

  addTo(compound, new CASTSpacer());

  addTo(compound, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("idl4_msgbuf_init"),
      new CASTUnaryOp("&",
        new CASTIdentifier("msgbuf"))))
  );
      
  CASTExpression *bufferExpr = new CASTFunctionOp(
    new CASTIdentifier((debug_mode&DEBUG_TESTSUITE) ? "smalloc" : "malloc"),
    new CASTIntegerConstant(SERVER_BUFFER_SIZE)
  );
  
  addTo(compound, new CASTForStatement(
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTIdentifier("cnt"),
        new CASTIntegerConstant(0))),
    new CASTBinaryOp("<",
      new CASTIdentifier("cnt"),
      strbufSize->clone()),
    new CASTPostfixOp("++",
      new CASTIdentifier("cnt")),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("idl4_msgbuf_add_buffer"),
        knitExprList(
          new CASTUnaryOp("&",
            new CASTIdentifier("msgbuf")),
          bufferExpr,
          new CASTIntegerConstant(SERVER_BUFFER_SIZE)))))
  );

  if (debug_mode&DEBUG_TESTSUITE)
    {
      addTo(compound, new CASTSpacer());
      addTo(compound, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("idl4_msgbuf_set_rcv_window"),
          knitExprList(
            new CASTUnaryOp("&",
              new CASTIdentifier("msgbuf")),
            new CASTFunctionOp(
              new CASTIdentifier("idl4_get_default_rcv_window")))))
      );
    }
  
  addTo(compound, new CASTSpacer());
  
  CASTExpression *replyAndWaitArgs = NULL;
  addTo(replyAndWaitArgs, new CASTUnaryOp("&", new CASTIdentifier("partner")));
  addTo(replyAndWaitArgs, new CASTUnaryOp("&", new CASTIdentifier("msgtag")));
  addTo(replyAndWaitArgs, new CASTUnaryOp("&", new CASTIdentifier("msgbuf")));
  addTo(replyAndWaitArgs, new CASTUnaryOp("&", new CASTIdentifier("cnt")));
  
  CASTStatement *innerLoop = NULL;
  addTo(innerLoop, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("idl4_msgbuf_sync"),
      new CASTUnaryOp("&",
        new CASTIdentifier("msgbuf"))))
  );
  addTo(innerLoop, new CASTSpacer());
  addTo(innerLoop, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("idl4_reply_and_wait"),
      replyAndWaitArgs))
  );
  addTo(innerLoop, new CASTSpacer());
  addTo(innerLoop, new CASTIfStatement(
    new CASTFunctionOp(
      new CASTIdentifier("idl4_is_error"),
      new CASTUnaryOp("&",
        new CASTIdentifier("msgtag"))),
    new CASTBreakStatement())
  );    
  addTo(innerLoop, new CASTSpacer());

  CASTExpression *processRequestArgs = NULL;
  addTo(processRequestArgs, new CASTUnaryOp("&", new CASTIdentifier("partner")));
  addTo(processRequestArgs, new CASTUnaryOp("&", new CASTIdentifier("msgtag")));
  addTo(processRequestArgs, new CASTUnaryOp("&", new CASTIdentifier("msgbuf")));
  addTo(processRequestArgs, new CASTUnaryOp("&", new CASTIdentifier("cnt")));

  CASTExpression *indexTable = utableRef;
  if (useItable)
    indexTable = new CASTIndexOp(
      indexTable, 
      new CASTBinaryOp("&",
        new CASTFunctionOp(
          new CASTIdentifier("idl4_get_interface_id"),
          new CASTUnaryOp("&",
            new CASTIdentifier("msgtag"))),
        iidMask)
    );
    
  addTo(processRequestArgs, new CASTIndexOp(
    indexTable,
    new CASTBinaryOp("&",
      new CASTFunctionOp(
        new CASTIdentifier("idl4_get_function_id"),
        new CASTUnaryOp("&",
          new CASTIdentifier("msgtag"))),
      fidMask))
  );    

  CASTStatement *dispatchStatement = new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("idl4_process_request"),
      processRequestArgs)
  );

  if (hasKernelMessages)
    {
      CASTExpression *kernelMsgArgs = NULL;
      addTo(kernelMsgArgs, new CASTUnaryOp("&", new CASTIdentifier("partner")));
      addTo(kernelMsgArgs, new CASTUnaryOp("&", new CASTIdentifier("msgtag")));
      addTo(kernelMsgArgs, new CASTUnaryOp("&", new CASTIdentifier("msgbuf")));
      addTo(kernelMsgArgs, new CASTUnaryOp("&", new CASTIdentifier("cnt")));
      addTo(kernelMsgArgs, new CASTIndexOp(
        ktableRef->clone(),
        new CASTBinaryOp("&",
          new CASTFunctionOp(
            new CASTIdentifier("idl4_get_kernel_message_id"),
            new CASTIdentifier("msgtag")),
          kidMask))
      );
      
      CASTStatement *kernelMsgBranch = new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("idl4_process_request"),
          kernelMsgArgs)
      );
    
      if (utableRef)
        dispatchStatement = new CASTIfStatement(
          new CASTFunctionOp(
            new CASTIdentifier("IDL4_EXPECT_FALSE"),
            new CASTFunctionOp(
              new CASTIdentifier("idl4_is_kernel_message"),
              new CASTIdentifier("msgtag"))),
          kernelMsgBranch,
          dispatchStatement
        );
        else dispatchStatement = kernelMsgBranch;
    }

  addTo(innerLoop, dispatchStatement);

  CASTStatement *outerLoop = NULL;
  addTo(outerLoop,
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTIdentifier("partner"),
        new CASTIdentifier("L4_nilthread")))
  );
  addTo(outerLoop,
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTBinaryOp(".",
          new CASTIdentifier("msgtag"),
          new CASTIdentifier("raw")),
        new CASTIntegerConstant(0)))
  );
  addWithTrailingSpacerTo(outerLoop,
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTIdentifier("cnt"),
        new CASTIntegerConstant(0)))
  );
  addTo(outerLoop,
    new CASTWhileStatement(
      new CASTIntegerConstant(1),
      new CASTCompoundStatement(innerLoop))
  );
  if (debug_mode&DEBUG_TESTSUITE)
    addWithLeadingSpacerTo(outerLoop,
      new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("enter_kdebug"),
          new CASTStringConstant(false, "message error")))
    );
  
  addTo(compound,
    new CASTWhileStatement(
      new CASTIntegerConstant(1),
      new CASTCompoundStatement(outerLoop))
  );
  
  return compound;
}
