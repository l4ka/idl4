#include "be.h"
#include "ms.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_GENERATOR) println(a); } while (0)
#define prependStatement(a, b) do { CASTStatement *pre = (b); addTo(pre, (a)); (a) = (pre); } while (0)

static CASTStatement *buildMemoryLeakCheck()

{
  return new CASTIfStatement(
    new CASTFunctionOp(
      new CASTIdentifier("firstalloc")),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
          knitExprList(
            new CASTStringConstant(false, "Memory leak detected at %Xh"),
            new CASTFunctionOp(
              new CASTIdentifier("firstalloc"))))));
}

CASTIdentifier *CBEOperation::buildTestFuncName()

{
  CASTIdentifier *testFuncName = buildOriginalClassIdentifier();
  testFuncName->addPrefix("test_");
  testFuncName->addPostfix("_");
  testFuncName->addPostfix(aoi->name);
  
  return testFuncName;
}  

CASTStatement *CBEOperation::buildTestDeclarations()

{
  CASTStatement *xferElements = NULL;
  if (!aoi->returnType->isVoid())
    addTo(xferElements, new CASTDeclarationStatement(
      getType(aoi->returnType)->buildDeclaration(
        new CASTDeclarator(new CASTIdentifier("__retval"))))
    );
    
  forAll(aoi->parameters, 
    addTo(xferElements, new CASTDeclarationStatement(
      getType(((CAoiParameter*)item)->type)->buildDeclaration(
        new CASTDeclarator(new CASTIdentifier(item->name)))))
  );

  CASTStatement *otherElements = NULL;
  addTo(otherElements, new CASTDeclarationStatement(
    msFactory->getThreadIDType()->buildDeclaration(
      new CASTDeclarator(new CASTIdentifier("caller"))))
  );
  
  CASTDeclarator *flagDeclarator = new CASTDeclarator(
    new CASTIdentifier("flag")
  );
  
  flagDeclarator->setQualifiers(new CASTConstOrVolatileSpecifier("volatile"));
  addTo(otherElements, new CASTDeclarationStatement(
    msFactory->getMachineWordType()->buildDeclaration(
      flagDeclarator))
  );
    
  CASTStatement *elements = NULL;
  if (xferElements)
    addTo(elements, new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTAggregateSpecifier("struct", NULL, xferElements),
        knitDeclList(  
          new CASTDeclarator(new CASTIdentifier("client")),
          new CASTDeclarator(new CASTIdentifier("svr_in")),
          new CASTDeclarator(new CASTIdentifier("svr_out")))))
    );
  addTo(elements, otherElements);
        
  return new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTAggregateSpecifier("struct", NULL, elements),
      new CASTDeclarator(buildTestKeyPrefix()))
  );
}  

CASTStatement *CBEOperation::buildTestFunction()

{
  CASTExpression *pathToItem = new CASTBinaryOp(".", 
    new CASTBinaryOp(".",
      new CASTIdentifier("globals"), 
      buildTestKeyPrefix()),
    new CASTIdentifier("*")
  );  
  CASTExpression *returnPath = new CASTBinaryOp(".",
    pathToItem->clone(),
    new CASTIdentifier("__retval")
  );

  CASTStatement *compound = NULL;

  /* Identify the function being tested */

  CASTStatement *notification = NULL;
  addTo(notification, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("printf"),
      new CASTStringConstant(false, mprintf("Testing %s::%s...\\n", ((CAoiInterface*)aoi->parent)->scopedName, aoi->name))))
  );    

  /* Declare local variables */
  
  CASTStatement *localVars = NULL;
  addTo(localVars, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTTypeSpecifier(new CASTIdentifier("CORBA_Environment")),
      new CASTDeclarator(
        new CASTIdentifier("_env"),
        NULL,
        NULL,
        new CASTIdentifier("idl4_default_environment"))))
  );    
  addTo(localVars, new CASTDeclarationStatement(
    msFactory->getMachineWordType()->buildDeclaration(
      new CASTDeclarator(
        new CASTIdentifier("_pass"))))
  );

  /* Initialize IN and INOUT parameters */

  CASTStatement *initStatements = NULL;

  CBEVarSource *varSource = new CBEVarSource();  
  forAllBE(getOperation(aoi)->sortedParams, 
    addTo(initStatements, ((CBEParameter*)item)->buildTestClientInit(pathToItem->clone(), varSource))
  );
  
  bool involvesMapping = false;
  forAllBE(getOperation(aoi)->sortedParams,
    if (getType(((CBEParameter*)item)->aoi->type)->involvesMapping())
      if (((CBEParameter*)item)->isSentFromServerToClient())
        involvesMapping = true
  );
  if (getType(aoi->returnType)->involvesMapping())
    involvesMapping = true;
      
  if (involvesMapping)
    addTo(initStatements, new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("idl4_set_rcv_window"),
        knitExprList(
          new CASTUnaryOp("&",
            new CASTIdentifier("_env")),
          new CASTFunctionOp(
            new CASTIdentifier("idl4_get_default_rcv_window")))))
    );

  /* Tell the server function to return normally (i.e. not raise an exception) */

  CASTStatement *globalsInit = NULL;
  CASTExpression *pathToGlobals = new CASTBinaryOp(".", 
    new CASTIdentifier("globals"), 
    buildTestKeyPrefix()
  );
  CASTExpression *pathToFlag = new CASTBinaryOp(".",
    pathToGlobals->clone(),
    new CASTIdentifier("flag")
  );
  
  addTo(globalsInit, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      new CASTBinaryOp(".",
        pathToGlobals->clone(),
        new CASTIdentifier("caller")),
      new CASTIdentifier("myself")))
  );

  /* Multiple passes are necessary to test the different stubs.
     Identify the pass we are currently executing. */

  CASTStatement *switchBody = NULL;
  addTo(switchBody, new CASTCaseStatement(
    knitStmtList(
      new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("printf"),
          new CASTStringConstant(false, " - Standard call\\n"))),
      new CASTExpressionStatement(
        new CASTBinaryOp("=",
          pathToFlag->clone(),
          new CASTIdentifier("CMD_RUN"))),
      new CASTBreakStatement()),
    new CASTIntegerConstant(0))
  );
  
  if (!(aoi->flags&FLAG_ONEWAY))
    addTo(switchBody, new CASTCaseStatement(
      knitStmtList(
        new CASTExpressionStatement(
          new CASTFunctionOp(
            new CASTIdentifier("printf"),
            new CASTStringConstant(false, " - Reply stub\\n"))),
        new CASTExpressionStatement(
          new CASTBinaryOp("=",
            pathToFlag->clone(),
            new CASTIdentifier("CMD_DELAY"))),
        new CASTBreakStatement()),
      new CASTIntegerConstant(1))
    );
    
  addTo(switchBody, new CASTCaseStatement(
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(
          new CASTStringConstant(false, "Invalid pass: %d"),
          new CASTIdentifier("_pass")))),
    new CASTDefaultConstant())
  );
    
  addTo(globalsInit, new CASTSwitchStatement(
    new CASTIdentifier("_pass"),
    new CASTCompoundStatement(switchBody))
  );

  /* After the call, check for exceptions. These include system exceptions
     (e.g. cut_message) and user-defined exceptions. Neither should have 
     occurred at this point because we told the server to return normally */

  CASTStatement *excCheckStatements = NULL;
  CASTExpression *exceptionMajor = 
    new CASTBinaryOp(".",
      new CASTIdentifier("_env"),
        new CASTIdentifier("_major"));
  CASTExpression *exceptionMinor = 
    new CASTBinaryOp(".",
      new CASTIdentifier("_env"),
        new CASTIdentifier("_minor"));
  
  addTo(excCheckStatements, new CASTIfStatement(
    new CASTBinaryOp("!=",
      exceptionMajor->clone(),
      new CASTIntegerConstant(0)),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(new CASTStringConstant(false, mprintf("Exception occurred: %%d.%%d")),
                     exceptionMajor->clone(), exceptionMinor->clone()))))
  );      

  /* Check whether the server function has run to completion.
     This is particularly important for ONEWAY functions, because they do
     not return a reply; in this case, we yield to the server until it
     completes. */

  CASTStatement *flagCheck = NULL;
  if (aoi->flags&FLAG_ONEWAY)
    {
      addTo(flagCheck, new CASTWhileStatement(
        new CASTBinaryOp("==",
          pathToFlag->clone(),
          new CASTIdentifier("CMD_RUN")),
        new CASTExpressionStatement(
          new CASTFunctionOp(
            new CASTIdentifier("idl4_yield_to"),
            new CASTIdentifier("server"))))
      );
    }
    
  addTo(flagCheck, new CASTIfStatement(
    new CASTBinaryOp("||",
      new CASTBrackets(
        new CASTBinaryOp("&&",
          new CASTBinaryOp("==",
            new CASTIdentifier("_pass"),
            new CASTIntegerConstant(0)),
          new CASTBinaryOp("!=",
            pathToFlag->clone(),
            new CASTIdentifier("STATUS_OK")))),
      new CASTBrackets(
        new CASTBinaryOp("&&",
          new CASTBinaryOp("==",
            new CASTIdentifier("_pass"),
            new CASTIntegerConstant(1)),
          new CASTBinaryOp("!=",
            pathToFlag->clone(),
            new CASTIdentifier("STATUS_DELAYED"))))),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        knitExprList(
          new CASTStringConstant(false, mprintf("Function returns with incorrect status: %%Xh (expected %%Xh)")),
          pathToFlag->clone(),
          new CASTConditionalExpression(
            new CASTBrackets(
              new CASTBinaryOp("==",
                new CASTIdentifier("_pass"),
                new CASTIntegerConstant(0))),
            new CASTIdentifier("STATUS_OK"),
            new CASTIdentifier("STATUS_DELAYED"))))))
  );

  /* Check all INOUT and OUT parameters */

  CASTStatement *checkStatements = NULL;
  forAllBE(getOperation(aoi)->sortedParams, 
    addTo(checkStatements, ((CBEParameter*)item)->buildTestClientCheck(pathToItem->clone(), varSource))
  );
  if (!aoi->returnType->isVoid())
    addTo(checkStatements, getType(aoi->returnType)->buildTestClientCheck(
      returnPath->clone(), new CASTIdentifier("__retval"), 
      varSource, OUT)
    );

  /* Free buffers and local variables */

  CASTStatement *postStatements = NULL;
  forAllBE(getOperation(aoi)->sortedParams, 
    {
      addTo(postStatements, ((CBEParameter*)item)->buildTestClientPost(pathToItem->clone(), varSource));
      addTo(postStatements, ((CBEParameter*)item)->buildTestClientCleanup(pathToItem->clone(), varSource));
    }  
  );
  if (!aoi->returnType->isVoid())
    {
      addTo(postStatements, getType(aoi->returnType)->buildTestClientPost(
        returnPath->clone(), new CASTIdentifier("__retval"), 
        varSource, OUT)
      );
      addTo(postStatements, getType(aoi->returnType)->buildTestClientCleanup(
        returnPath->clone(), new CASTIdentifier("__retval"), 
        varSource, OUT)
      );
    }  

  /* Test all user-defined exceptions */

  CASTStatement *excStatements = NULL;
  forAll(aoi->exceptions,
    {
      CAoiExceptionType *exc = (CAoiExceptionType*)((CAoiRef*)item)->ref;
      
      // Print a message indicating which exception is being tested
      
      addWithLeadingSpacerTo(excStatements, new CASTBlockComment(mprintf("Exception %s::%s", ((CAoiInterface*)exc->parentScope)->scopedName, exc->name)));
      assert(exc->parentScope);
      addTo(excStatements, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("printf"),
          new CASTStringConstant(false, mprintf(" - Raising %s::%s\\n", ((CAoiInterface*)exc->parentScope)->scopedName, exc->name))))
      );    
      
      // Tell the server which exception to raise
      
      addWithTrailingSpacerTo(excStatements, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          pathToFlag->clone(),
          getException(exc)->buildID()))
      );

      // Initialize all IN and INOUT parameters

      forAllBE(getOperation(aoi)->sortedParams, 
        addTo(excStatements, ((CBEParameter*)item)->buildTestClientInit(pathToItem->clone(), varSource))
      );
      
      // Invoke the service
      
      addWithLeadingSpacerTo(excStatements, buildTestInvocation());
      
      // Check whether the exception has actually occurred
      
      addWithLeadingSpacerTo(excStatements, new CASTIfStatement(
        new CASTBinaryOp("||",
          new CASTBinaryOp("!=",
            exceptionMajor->clone(),
            new CASTIdentifier("CORBA_USER_EXCEPTION")),
          new CASTBinaryOp("!=",
            exceptionMinor->clone(),
            getException(exc)->buildID())),  
        new CASTExpressionStatement(
          new CASTFunctionOp(
            new CASTIdentifier("panic"),
            knitExprList(
              new CASTStringConstant(false, mprintf("Exception reported: %%d.%%d (expected %%d.%%d)")),
              exceptionMajor->clone(),
              exceptionMinor->clone(),
              new CASTIdentifier("CORBA_USER_EXCEPTION"),
              getException(exc)->buildID()))))
      );
      
      // Check whether the server has run to completion
      
      addTo(excStatements, new CASTIfStatement(
        new CASTBinaryOp("!=",
          pathToFlag->clone(),
          new CASTBinaryOp("+",
            new CASTIdentifier("STATUS_EXCEPTION"),
            getException(exc)->buildID())),
        new CASTExpressionStatement(
          new CASTFunctionOp(
            new CASTIdentifier("panic"),
            knitExprList(
              new CASTStringConstant(false, mprintf("Function returns with incorrect status: %%Xh (expected %%Xh)")),
              pathToFlag->clone(),
              new CASTBinaryOp("+",
                new CASTIdentifier("STATUS_EXCEPTION"),
                getException(exc)->buildID())))))
      );
      
      // Release any state associated with the exception
      
      addTo(excStatements, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("CORBA_exception_free"),
          new CASTUnaryOp("&",
            new CASTIdentifier("_env"))))
      );


      // Free buffers and local variables

      forAllBE(getOperation(aoi)->sortedParams, 
        addTo(excStatements, ((CBEParameter*)item)->buildTestClientCleanup(pathToItem->clone(), varSource))
      );
      
      if (!aoi->returnType->isVoid())
        addTo(excStatements, getType(aoi->returnType)->buildTestClientCleanup(
          returnPath->clone(), new CASTIdentifier("__retval"), 
          varSource, OUT)
        );

      // Check for memory leaks

      addTo(excStatements, buildMemoryLeakCheck());
    }
  );

  /* Assemble the inner loop */
  
  CASTStatement *innerLoop = NULL;
  addWithTrailingSpacerTo(innerLoop, initStatements);
  addWithTrailingSpacerTo(innerLoop, globalsInit);
  addTo(innerLoop, buildTestInvocation());
  addWithLeadingSpacerTo(innerLoop, excCheckStatements);
  addWithLeadingSpacerTo(innerLoop, flagCheck);
  addWithLeadingSpacerTo(innerLoop, checkStatements);
  addWithLeadingSpacerTo(innerLoop, postStatements);
  addWithLeadingSpacerTo(innerLoop, buildMemoryLeakCheck());

  /* Assemble the outer loop */
  
  CASTStatement *outerLoop = NULL;
  addTo(outerLoop, new CASTForStatement(
    new CASTExpressionStatement(
      new CASTBinaryOp("=",
        new CASTIdentifier("_pass"),
        new CASTIntegerConstant(0))),
    new CASTBinaryOp("<",
      new CASTIdentifier("_pass"),
      new CASTIntegerConstant((aoi->flags&FLAG_ONEWAY) ? 1 : 2)),
    new CASTPostfixOp("++",
      new CASTIdentifier("_pass")),
    new CASTCompoundStatement(innerLoop))
  );

  /* Assemble the test function compound */
   
  addTo(localVars, varSource->declareAll());
  addWithTrailingSpacerTo(compound, localVars); 
  addTo(compound, notification);
  addWithTrailingSpacerTo(compound, outerLoop);
  addTo(compound, excStatements);
  
  /* Return the function */
  
  CASTStatement *result = NULL;
  addWithTrailingSpacerTo(result, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTTypeSpecifier(new CASTIdentifier("void")),
      new CASTDeclarator(
        buildTestFuncName(),
        new CASTDeclaration(
          new CASTTypeSpecifier(new CASTIdentifier("CORBA_Object")),
          new CASTDeclarator(new CASTIdentifier("server")))),
      new CASTCompoundStatement(compound)))
  );
  
  return result;
}

CASTStatement *CBEOperation::buildTestClientCode(CBEVarSource *varSource)

{
  CASTStatement *result = NULL;
  
  CASTExpression *globalPath = new CASTBinaryOp(".", 
    new CASTBinaryOp(".",
      new CASTIdentifier("globals"), 
      buildTestKeyPrefix()),
    new CASTIdentifier("*")
  );

  CASTExpression *expectedID = new CASTBinaryOp(".",
    new CASTBinaryOp(".",
      new CASTIdentifier("globals"),
      buildTestKeyPrefix()),
    new CASTIdentifier("caller")
  );

  addWithTrailingSpacerTo(result, new CASTIfStatement(
    new CASTUnaryOp("!",
      new CASTFunctionOp(
        new CASTIdentifier("idl4_threads_equal"),
        knitExprList(
          expectedID->clone(), 
          new CASTIdentifier("_caller")))),
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("panic"),
        new CASTStringConstant(false, "Caller ID incorrect"))))
  );

  CASTStatement *serverCheck = NULL, *serverInit = NULL, *serverRecheck = NULL;
      
  dprintln(" +  Building test server checks");
  forAllBE(getOperation(aoi)->sortedParams, 
    addTo(serverCheck, ((CBEParameter*)item)->buildTestServerCheck(globalPath->clone(), varSource))
  );
  if (serverCheck)
    prependStatement(serverCheck, new CASTBlockComment(mprintf("check in values")));

  addWithTrailingSpacerTo(result, serverCheck);

  dprintln(" +  Building test server init");
  forAllBE(getOperation(aoi)->sortedParams, 
    addTo(serverInit, ((CBEParameter*)item)->buildTestServerInit(globalPath->clone(), varSource, true))
  );
  if (serverInit)
    prependStatement(serverInit, new CASTBlockComment(mprintf("assign out values")));

  dprintln(" +  Building test server recheck");
  forAllBE(getOperation(aoi)->sortedParams, 
    addTo(serverRecheck, ((CBEParameter*)item)->buildTestServerRecheck(globalPath->clone(), varSource))
  );
  if (serverRecheck)
    prependStatement(serverRecheck, new CASTBlockComment(mprintf("check for collisions")));
  addWithLeadingSpacerTo(serverInit, serverRecheck);
  if (!aoi->returnType->isVoid())
    {
      CASTExpression *pathToReturnValue = new CASTBinaryOp(".",
        globalPath->clone(),
        new CASTIdentifier("__retval")
      );  
      dprintln(" +  Building return value test");
      addTo(serverInit, getType(aoi->returnType)->buildTestServerInit(
        pathToReturnValue, new CASTIdentifier("__retval"), varSource, OUT, NULL, false));
    }
        
  CASTStatement *serverFlag = NULL;
  CASTExpression *pathToFlag = new CASTBinaryOp(".", 
    new CASTBinaryOp(".",
      new CASTIdentifier("globals"), 
      buildTestKeyPrefix()),
    new CASTIdentifier("flag"));

  addWithLeadingSpacerTo(serverInit, new CASTBlockComment(mprintf("report to parent")));
      
  CASTStatement *switchBlock = NULL;
  addTo(switchBlock, new CASTCaseStatement(
    knitStmtList(
      new CASTExpressionStatement(
        new CASTBinaryOp("=",
          pathToFlag,
          new CASTIdentifier("STATUS_OK"))),
      new CASTBreakStatement()),
    new CASTIdentifier("CMD_RUN"))
  );
      
  CASTStatement *switchBody = NULL;
  if (!(aoi->flags&FLAG_ONEWAY))
    {
      CASTIdentifier *replyStubName = buildIdentifier();
      replyStubName->addPostfix("_reply");

      CASTExpression *replyStubArgs = NULL;
      addTo(replyStubArgs, new CASTIdentifier("_caller"));
      forAll(aoi->parameters, 
        if (getParameter(item)->isSentFromServerToClient())
          addTo(replyStubArgs, new CASTIdentifier(item->name))
      );
      if (!aoi->returnType->isVoid())
        addTo(replyStubArgs, new CASTIdentifier("__retval"));

      CASTStatement *delayedStmts = NULL;
      addTo(delayedStmts, new CASTExpressionStatement(
        new CASTBinaryOp("=",
          pathToFlag,
          new CASTIdentifier("STATUS_DELAYED")))
      );
      addTo(delayedStmts, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("idl4_set_no_response"),
          new CASTIdentifier("_env")))
      );
      addTo(delayedStmts, new CASTExpressionStatement(
        new CASTFunctionOp(
          replyStubName,
          replyStubArgs))
      );
      addTo(delayedStmts, new CASTBreakStatement());
      
      addTo(switchBlock, new CASTCaseStatement(
        delayedStmts,
        new CASTIdentifier("CMD_DELAY"))
      );
    }
        
  addTo(serverInit, new CASTSwitchStatement(
    pathToFlag->clone(),
    new CASTCompoundStatement(switchBlock))
  );
      
  addTo(serverInit, new CASTBreakStatement());
        
  CASTExpression *allowedFlags = new CASTIdentifier("CMD_RUN");
  if (!(aoi->flags&FLAG_ONEWAY))
    addTo(allowedFlags, new CASTIdentifier("CMD_DELAY"));
        
  addTo(switchBody, new CASTCaseStatement(
    knitStmtList(
      new CASTSpacer(),
      serverInit),
    allowedFlags)
  );
      
  forAll(aoi->exceptions,
    {
      CAoiRef *ref = (CAoiRef*)item;
      CASTStatement *excCompound = NULL;
         
      addTo(excCompound, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("CORBA_exception_set"),
          knitExprList(
            new CASTIdentifier("_env"),
            getException(ref->ref)->buildID(),
            new CASTIdentifier("NULL")
          )))
      );
          
      addTo(excCompound, new CASTExpressionStatement(
        new CASTBinaryOp("=", 
          pathToFlag->clone(),
          new CASTBinaryOp("+",
            new CASTIdentifier("STATUS_EXCEPTION"),
            getException(ref->ref)->buildID())))
      );
          
      addTo(excCompound, new CASTBreakStatement());
         
      addWithLeadingSpacerTo(switchBody, new CASTCaseStatement(
        excCompound,
        getException(ref->ref)->buildID())
      );
    }
  );
      
  addWithTrailingSpacerTo(result, new CASTSwitchStatement(
    pathToFlag->clone(),
    new CASTCompoundStatement(switchBody))
  );
          
  addWithTrailingSpacerTo(result, serverFlag);
      
  return result;
}

CASTIdentifier *CBEOperation::buildTestKeyPrefix()

{
  CASTIdentifier *id = buildOriginalClassIdentifier();
  id->addPostfix("_");
  id->addPostfix(aoi->name);

  return id;
}

CASTStatement *CBEOperation::buildTestInvocation()

{
  CASTExpression *pathToItem = new CASTBinaryOp(".", 
    new CASTBinaryOp(".",
      new CASTIdentifier("globals"), 
      buildTestKeyPrefix()),
    new CASTIdentifier("*")
  );  
  CASTExpression *returnPath = new CASTBinaryOp(".",
    pathToItem->clone(),
    new CASTIdentifier("__retval")
  );

  bool hasImplicitHandle = false;
  forAll(aoi->parameters, 
    if (getParameter(item)->getFirstMemberWithProperty("handle")) 
      hasImplicitHandle = true
  );

  CASTExpression *arguments = NULL;
  if (!hasImplicitHandle)
    addTo(arguments, new CASTIdentifier("server"));
    
  forAll(aoi->parameters, addTo(arguments, 
    getParameter(item)->buildTestClientArg(
      new CASTBinaryOp(".",
        new CASTBinaryOp(".",
          new CASTIdentifier("globals"), 
          buildTestKeyPrefix()),
        new CASTIdentifier("client"))  
    ))
  );
  addTo(arguments, new CASTUnaryOp("&", new CASTIdentifier("_env")));
  
  CASTExpression *invocation = new CASTFunctionOp(buildIdentifier(), arguments);
  if (!aoi->returnType->isVoid())
    invocation = new CASTBinaryOp("=",
      knitPathInstance(returnPath, "client"),
      invocation
    );
    
  return new CASTExpressionStatement(invocation);
}
