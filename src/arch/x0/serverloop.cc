#include "cast.h"
#include "arch/x0.h"

CASTStatement *CMSServiceX::buildServerLoop(CASTIdentifier *prefix, CASTExpression *utableRef, CASTExpression *ktableRef, bool useItable, bool hasKernelMessages)

{
  CBEType *u32Type = new CBEOpaqueType("unsigned int", 4, 4);
  CASTIdentifier *msgbufSize = prefix->clone();
  CASTIdentifier *strbufSize = prefix->clone();
  CASTIdentifier *rcvDope = prefix->clone();
  CASTIdentifier *fidMask = prefix->clone();
  CASTIdentifier *iidMask = prefix->clone();
  CASTStatement *compound = NULL;

  if (hasKernelMessages)
    warning("The X0 and V2 backends do not support kernel message dispatching");
  
  msgbufSize->addPostfix("_MSGBUF_SIZE");
  strbufSize->addPostfix("_STRBUF_SIZE");
  rcvDope->addPostfix("_RCV_DOPE");
  fidMask->addPostfix("_FID_MASK");
  iidMask->addPostfix("_IID_MASK");

  bool hasInFpages = false;
  forAllMS(connections,
    if (((CMSConnectionX*)item)->hasFpagesInChannel(CHANNEL_IN))
      hasInFpages = true
  );

  CASTExpression *rcvWindow;
  if ((debug_mode&DEBUG_TESTSUITE) && hasInFpages)
    rcvWindow = new CASTFunctionOp(new CASTIdentifier("idl4_get_default_rcv_window")); 
    else rcvWindow = new CASTIdentifier("idl4_nilpage");
  
  CASTDeclarator *localIntVars = NULL;
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("w0")));
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("w1")));
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("w2"))); 
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("send_desc")));
  if ((debug_mode&DEBUG_TESTSUITE) && (debug_mode&DEBUG_PARANOID))
    addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("i")));
  
  addTo(compound,
    new CASTDeclarationStatement(
      u32Type->buildDeclaration(
        localIntVars))
  );
  
  addTo(compound,
    new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTTypeSpecifier(new CASTIdentifier("l4_msgdope_t")),
        new CASTDeclarator(new CASTIdentifier("msgdope"))))
  );

  if ((numStrings>0) && (debug_mode&DEBUG_TESTSUITE) && (debug_mode&DEBUG_PARANOID))
    addTo(compound, new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTTypeSpecifier(new CASTIdentifier("idl4_strdope_t")),
        new CASTDeclarator(
          new CASTIdentifier("str"),
          NULL,
          strbufSize)))
    );

  addTo(compound,
    new CASTDeclarationStatement(
      msFactory->getThreadIDType()->buildDeclaration(
        new CASTDeclarator(new CASTIdentifier("partner"))))
  );

  CASTDeclarationStatement *bufferMembers = NULL;
  addTo(bufferMembers, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTTypeSpecifier("l4_fpage_t"),
      new CASTDeclarator(
        new CASTIdentifier("rcv_window"))))
  );
  addTo(bufferMembers, new CASTDeclarationStatement(
    u32Type->buildDeclaration(
      new CASTDeclarator(
        new CASTIdentifier("size_dope"))))
  );
  addTo(bufferMembers, new CASTDeclarationStatement(
    u32Type->buildDeclaration(
      new CASTDeclarator(
        new CASTIdentifier("send_dope"))))
  );
  addTo(bufferMembers, new CASTDeclarationStatement(
    u32Type->buildDeclaration(
      new CASTDeclarator(
        new CASTIdentifier("message"),
        NULL,
        msgbufSize)))
  );
  
  addTo(bufferMembers, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTTypeSpecifier(new CASTIdentifier("idl4_strdope_t")),
      new CASTDeclarator(
        new CASTIdentifier("str"),
        NULL,
        strbufSize)))
  );
  
  addWithTrailingSpacerTo(compound,
    new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTAggregateSpecifier("struct", ANONYMOUS, bufferMembers),
        new CASTDeclarator(new CASTIdentifier("buffer"))))
  );
  
  if (numStrings>0)
    {
      CASTStatement *stringInitCompound = NULL;

      CASTExpression *bufferExpr = new CASTFunctionOp(
        new CASTIdentifier((debug_mode&DEBUG_TESTSUITE) ? "smalloc" : "malloc"),
        new CASTIntegerConstant(SERVER_BUFFER_SIZE)
      );
      
      CASTExpression *sizeExpr = new CASTIntegerConstant(SERVER_BUFFER_SIZE);
      
      if ((debug_mode&DEBUG_TESTSUITE) && (debug_mode&DEBUG_PARANOID))
        {
          bufferExpr = new CASTBinaryOp("=",
            new CASTBinaryOp(".",
              new CASTIndexOp(
                new CASTIdentifier("str"),
                new CASTIdentifier("w0")),
              new CASTIdentifier("rcv_addr")),
            bufferExpr
          );  
          
          sizeExpr = new CASTBinaryOp("=",
            new CASTBinaryOp(".",
              new CASTIndexOp(
                new CASTIdentifier("str"),
                new CASTIdentifier("w0")),
              new CASTIdentifier("rcv_size")),
            sizeExpr
          );  
        }

      addTo(stringInitCompound, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            new CASTIndexOp(
              new CASTBinaryOp(".",
                new CASTIdentifier("buffer"),
                new CASTIdentifier("str")),
              new CASTIdentifier("w0")),
            new CASTIdentifier("rcv_addr")),  
          bufferExpr))
      );
      
      addTo(stringInitCompound, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTBinaryOp(".",
            new CASTIndexOp(
              new CASTBinaryOp(".",
                new CASTIdentifier("buffer"),
                new CASTIdentifier("str")),
              new CASTIdentifier("w0")),
            new CASTIdentifier("rcv_size")),  
          sizeExpr))
      );
    
      addWithTrailingSpacerTo(compound, new CASTForStatement(
        new CASTExpressionStatement(
          new CASTBinaryOp("=",
            new CASTIdentifier("w0"),
            new CASTIntegerConstant(0))),
        new CASTBinaryOp("<",
          new CASTIdentifier("w0"),
          strbufSize->clone()),
        new CASTPostfixOp("++",
          new CASTIdentifier("w0")),
        new CASTCompoundStatement(stringInitCompound))
      );
    }
  
  CASTStatement *innerLoop = NULL;

  if ((debug_mode&DEBUG_TESTSUITE) && (debug_mode&DEBUG_PARANOID))
    {
      addTo(innerLoop, new CASTIfStatement(
        new CASTBinaryOp("!=",
          new CASTBinaryOp(".",
            new CASTIdentifier("buffer"),
            new CASTIdentifier("size_dope")),
          rcvDope->clone()),
        new CASTExpressionStatement(
          new CASTFunctionOp(
            new CASTIdentifier("panic"),
            knitExprList(
              new CASTStringConstant(false, mprintf("Size dope overwritten in %s: Changed from %%Xh to %%Xh", prefix->identifier)),
              rcvDope->clone(),
              new CASTBinaryOp(".",
                new CASTIdentifier("buffer"),
                new CASTIdentifier("size_dope"))))))
      );
      
      CASTExpression *rcvWindowExpected = new CASTFunctionOp(
        new CASTIdentifier("idl4_raw_fpage"),
        rcvWindow->clone()
      );
      CASTExpression *rcvWindowGot = new CASTFunctionOp(
        new CASTIdentifier("idl4_raw_fpage"),
        new CASTBinaryOp(".",
          new CASTIdentifier("buffer"),
          new CASTIdentifier("rcv_window"))
      );
      
      addTo(innerLoop, new CASTIfStatement(
        new CASTBinaryOp("!=",
          rcvWindowGot,
          rcvWindowExpected),
        new CASTExpressionStatement(
          new CASTFunctionOp(
            new CASTIdentifier("panic"),
            knitExprList(
              new CASTStringConstant(false, mprintf("Receive window overwritten in %s: Changed from %%Xh to %%Xh", prefix->identifier)),
              rcvWindowExpected->clone(),
              rcvWindowGot->clone()))))
      );

      CASTExpression *addrExpected = new CASTBinaryOp(".",
        new CASTIndexOp(
          new CASTIdentifier("str"),
          new CASTIdentifier("i")),
        new CASTIdentifier("rcv_addr")
      );
      CASTExpression *addrGot = new CASTBinaryOp(".",
        new CASTBinaryOp(".",
          new CASTIdentifier("buffer"),
          new CASTIndexOp(
            new CASTIdentifier("str"),
            new CASTIdentifier("i"))),
        new CASTIdentifier("rcv_addr")
      );
      CASTExpression *sizeExpected = new CASTBinaryOp(".",
        new CASTIndexOp(
          new CASTIdentifier("str"),
          new CASTIdentifier("i")),
        new CASTIdentifier("rcv_size")
      );
      CASTExpression *sizeGot = new CASTBinaryOp(".",
        new CASTBinaryOp(".",
          new CASTIdentifier("buffer"),
          new CASTIndexOp(
            new CASTIdentifier("str"),
            new CASTIdentifier("i"))),
        new CASTIdentifier("rcv_size")
      );

      CASTStatement *bufferTest = NULL;
      addTo(bufferTest, new CASTIfStatement(
        new CASTBinaryOp("!=",
          addrGot,
          addrExpected),
        new CASTExpressionStatement(
          new CASTFunctionOp(
            new CASTIdentifier("panic"),
              knitExprList(
                new CASTStringConstant(false, mprintf("Buffer address changed in receive dope %s:%%d: Is %%Xh, should be %%Xh", prefix->identifier)),
                new CASTIdentifier("i"),
                addrGot->clone(),
                addrExpected->clone()))))
      );
      addTo(bufferTest, new CASTIfStatement(
        new CASTBinaryOp("!=",
          sizeGot,
          sizeExpected),
        new CASTExpressionStatement(
          new CASTFunctionOp(
            new CASTIdentifier("panic"),
              knitExprList(
                new CASTStringConstant(false, mprintf("Buffer size changed in receive dope %s:%%d: Is %%Xh, should be %%Xh", prefix->identifier)),
                new CASTIdentifier("i"),
                sizeGot->clone(),
                sizeExpected->clone()))))
      );
      
      if (numStrings>0)
        addTo(innerLoop, new CASTForStatement(
          new CASTExpressionStatement(
            new CASTBinaryOp("=",
              new CASTIdentifier("i"),
              new CASTIntegerConstant(0))),
          new CASTBinaryOp("<",
            new CASTIdentifier("i"),
            strbufSize->clone()),
          new CASTPostfixOp("++",
            new CASTIdentifier("i")),
          new CASTCompoundStatement(bufferTest))
        );  

      addTo(innerLoop, new CASTSpacer());
    }
  
  CASTExpression *replyAndWaitArgs = NULL;
  addTo(replyAndWaitArgs, new CASTUnaryOp("&", new CASTIdentifier("partner")));
  addTo(replyAndWaitArgs, new CASTUnaryOp("&", new CASTIdentifier("buffer")));
  addTo(replyAndWaitArgs, new CASTUnaryOp("&", new CASTIdentifier("w0")));
  addTo(replyAndWaitArgs, new CASTUnaryOp("&", new CASTIdentifier("w1")));
  addTo(replyAndWaitArgs, new CASTUnaryOp("&", new CASTIdentifier("w2"))); 
  addTo(replyAndWaitArgs, new CASTIdentifier("send_desc"));
  
  addWithTrailingSpacerTo(innerLoop, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      new CASTIdentifier("msgdope"),
      new CASTFunctionOp(
        new CASTIdentifier("idl4_reply_and_wait"),
        replyAndWaitArgs)))
  );

  addWithTrailingSpacerTo(innerLoop, new CASTIfStatement(
    new CASTFunctionOp(
      new CASTIdentifier("L4_IPC_ERROR"),
      new CASTIdentifier("msgdope")),
    new CASTBreakStatement())
  );

  CASTExpression *processRequestArgs = NULL;
  
  CASTExpression *indexTable = utableRef;
  if (useItable)
    indexTable = new CASTIndexOp(
      indexTable, 
      new CASTBinaryOp("&",
        new CASTBrackets(
          new CASTBinaryOp(">>",
            new CASTIdentifier("w2"),
            new CASTIdentifier("IDL4_FID_BITS"))),
        iidMask)
    );

  addTo(processRequestArgs, new CASTIdentifier("partner"));
  addTo(processRequestArgs, new CASTUnaryOp("&", new CASTIdentifier("buffer")));
  addTo(processRequestArgs, new CASTUnaryOp("&", new CASTIdentifier("w0")));
  addTo(processRequestArgs, new CASTUnaryOp("&", new CASTIdentifier("w1")));
  addTo(processRequestArgs, new CASTUnaryOp("&", new CASTIdentifier("w2")));
  addTo(processRequestArgs, new CASTIndexOp(
    indexTable,
    new CASTBinaryOp("&", 
      new CASTIdentifier("w2"), 
      fidMask))
  );

  addTo(innerLoop, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      new CASTIdentifier("send_desc"),
      new CASTFunctionOp(
        new CASTIdentifier("idl4_process_request"),
        processRequestArgs)))
  );

  CASTStatement *outerLoop = NULL;
  addTo(outerLoop,
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTBinaryOp(".",
          new CASTIdentifier("buffer"),
          new CASTIdentifier("size_dope")),
        rcvDope->clone()))
  );
  
  addTo(outerLoop,
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTBinaryOp(".",
          new CASTIdentifier("buffer"),
          new CASTIdentifier("rcv_window")),
        rcvWindow->clone()))
  );
  addTo(outerLoop,
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTIdentifier("partner"),
        new CASTIdentifier("idl4_nilthread")))
  );
  addTo(outerLoop,
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTIdentifier("send_desc"),
        new CASTIdentifier("idl4_nil")))
  );
  addWithTrailingSpacerTo(outerLoop,
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTIdentifier("w0"),
        new CASTBinaryOp("=",
          new CASTIdentifier("w1"),
          new CASTBinaryOp("=",
            new CASTIdentifier("w2"),
            new CASTIntegerConstant(0)))))
  );

  addWithTrailingSpacerTo(outerLoop,
    new CASTWhileStatement(
      new CASTIntegerConstant(1),
      new CASTCompoundStatement(innerLoop))
  );

  addTo(outerLoop,
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("enter_kdebug"),
        new CASTStringConstant(false, mprintf("message error"))))
  );
  
  addTo(compound,
    new CASTWhileStatement(
      new CASTIntegerConstant(1),
      new CASTCompoundStatement(outerLoop))
  );
  
  return compound;
}
