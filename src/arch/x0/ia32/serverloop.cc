#include "cast.h"
#include "arch/x0.h"

CASTStatement *CMSServiceIX::buildServerLoop(CASTIdentifier *prefix, CASTExpression *utableRef, CASTExpression *ktableRef, bool useItable, bool hasKernelMessages)

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
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("msgdope")));
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("dummy")));
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("fnr")));
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("reply")));
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("w0")));
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("w1")));
  addTo(localIntVars, new CASTDeclarator(new CASTIdentifier("w2")));
  
  addTo(compound,
    new CASTDeclarationStatement(
      u32Type->buildDeclaration(
        localIntVars))
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
    u32Type->buildDeclaration(
      new CASTDeclarator(
        new CASTIdentifier("stack"),
        NULL,
        new CASTIntegerConstant(768))))
  );
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
  
  addTo(compound,
    new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTAggregateSpecifier("struct", ANONYMOUS, bufferMembers),
        new CASTDeclarator(new CASTIdentifier("buffer"))))
  );
  
  addTo(compound, new CASTSpacer());
  
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
    
      addTo(compound, new CASTForStatement(
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
              
      addTo(compound, new CASTSpacer());
    }
  
  CASTStatement *innerLoop = NULL;

  if ((debug_mode&DEBUG_TESTSUITE) && (debug_mode&DEBUG_PARANOID))
    {
      addTo(innerLoop, new CASTIfStatement(
        new CASTBinaryOp("!=",
          new CASTIndexOp(
            new CASTBinaryOp(".",
              new CASTIdentifier("buffer"),
              new CASTIdentifier("stack")),
            new CASTIntegerConstant(0)),
          new CASTIdentifier("IDL4_STACK_MAGIC")),
        new CASTExpressionStatement(
          new CASTFunctionOp(
            new CASTIdentifier("panic"),
            new CASTStringConstant(false, mprintf("Stack overflow in %s", prefix->identifier)))))
      );
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
          new CASTIdentifier("dummy")),
        new CASTIdentifier("rcv_addr")
      );
      CASTExpression *addrGot = new CASTBinaryOp(".",
        new CASTBinaryOp(".",
          new CASTIdentifier("buffer"),
          new CASTIndexOp(
            new CASTIdentifier("str"),
            new CASTIdentifier("dummy"))),
        new CASTIdentifier("rcv_addr")
      );
      CASTExpression *sizeExpected = new CASTBinaryOp(".",
        new CASTIndexOp(
          new CASTIdentifier("str"),
          new CASTIdentifier("dummy")),
        new CASTIdentifier("rcv_size")
      );
      CASTExpression *sizeGot = new CASTBinaryOp(".",
        new CASTBinaryOp(".",
          new CASTIdentifier("buffer"),
          new CASTIndexOp(
            new CASTIdentifier("str"),
            new CASTIdentifier("dummy"))),
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
                new CASTIdentifier("dummy"),
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
                new CASTIdentifier("dummy"),
                sizeGot->clone(),
                sizeExpected->clone()))))
      );
      
      if (numStrings>0)
        addTo(innerLoop, new CASTForStatement(
          new CASTExpressionStatement(
            new CASTBinaryOp("=",
              new CASTIdentifier("dummy"),
              new CASTIntegerConstant(0))),
          new CASTBinaryOp("<",
            new CASTIdentifier("dummy"),
            strbufSize->clone()),
          new CASTPostfixOp("++",
            new CASTIdentifier("dummy")),
          new CASTCompoundStatement(bufferTest))
        );  

      addTo(innerLoop, new CASTSpacer());
    }
  
  CASTExpression *replyAndWaitArgs = NULL;
  addTo(replyAndWaitArgs, new CASTIdentifier("reply"));
  addTo(replyAndWaitArgs, new CASTIdentifier("buffer"));
  addTo(replyAndWaitArgs, new CASTIdentifier("partner"));
  addTo(replyAndWaitArgs, new CASTIdentifier("msgdope"));
  addTo(replyAndWaitArgs, new CASTIdentifier("fnr"));
  addTo(replyAndWaitArgs, new CASTIdentifier("w0"));
  addTo(replyAndWaitArgs, new CASTIdentifier("w1"));
  addTo(replyAndWaitArgs, new CASTIdentifier("w2"));
  addTo(replyAndWaitArgs, new CASTIdentifier("dummy"));
  
  addTo(innerLoop, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("idl4_reply_and_wait"),
      replyAndWaitArgs))
  );
  addTo(innerLoop, new CASTSpacer());
  addTo(innerLoop, new CASTIfStatement(
    new CASTBinaryOp("&",
      new CASTIdentifier("msgdope"),
      new CASTHexConstant(0xF0)),
    new CASTBreakStatement())
  );    
  addTo(innerLoop, new CASTSpacer());

  CASTExpression *processRequestArgs = NULL;
  
  CASTExpression *indexTable = utableRef;
  if (useItable)
    indexTable = new CASTIndexOp(
      indexTable, 
      new CASTBinaryOp("&",
        new CASTBrackets(
          new CASTBinaryOp(">>",
            new CASTIdentifier("fnr"),
            new CASTIdentifier("IDL4_FID_BITS"))),
        iidMask)
    );
  addTo(processRequestArgs, indexTable);
  
  addTo(processRequestArgs, new CASTBinaryOp("&", 
    new CASTIdentifier("fnr"), 
    fidMask)
  );
  addTo(processRequestArgs, new CASTIdentifier("reply"));
  addTo(processRequestArgs, new CASTIdentifier("buffer"));
  addTo(processRequestArgs, new CASTIdentifier("partner"));
  addTo(processRequestArgs, new CASTIdentifier("w0"));
  addTo(processRequestArgs, new CASTIdentifier("w1"));
  addTo(processRequestArgs, new CASTIdentifier("w2"));
  addTo(processRequestArgs, new CASTIdentifier("dummy"));

  addTo(innerLoop, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("idl4_process_request"),
      processRequestArgs))
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
        new CASTIdentifier("reply"),
        new CASTIdentifier("idl4_nil")))
  );
  addTo(outerLoop,
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTIdentifier("w0"),
        new CASTBinaryOp("=",
          new CASTIdentifier("w1"),
          new CASTBinaryOp("=",
            new CASTIdentifier("w2"),
            new CASTIntegerConstant(0)))))
  );

  if ((debug_mode&DEBUG_TESTSUITE) && (debug_mode&DEBUG_PARANOID))
    addTo(outerLoop,
      new CASTExpressionStatement(
        new CASTBinaryOp("=",
          new CASTIndexOp(
            new CASTBinaryOp(".",
              new CASTIdentifier("buffer"),
              new CASTIdentifier("stack")),
            new CASTIntegerConstant(0)),
          new CASTIdentifier("IDL4_STACK_MAGIC")))
    );  
  
  addTo(outerLoop, new CASTSpacer());
  addTo(outerLoop,
    new CASTWhileStatement(
      new CASTIntegerConstant(1),
      new CASTCompoundStatement(innerLoop))
  );
  addTo(outerLoop, new CASTSpacer());
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
