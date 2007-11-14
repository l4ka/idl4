#include "be.h"
#include "ops.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_GENERATOR) println(a); } while (0)
#define prependStatement(a, b) do { CASTStatement *pre = (b); addTo(pre, (a)); (a) = (pre); } while (0)

char contractBuffer[20000];


void CBEOperation::check()

{
  CBEType *type;

  CAoiType *returnType = aoi->returnType;
  if (!returnType->isVoid())
    {
      type = getType(returnType);
      if (type && !type->isAllowedParamType())
        semanticError(aoi->context, "not an allowed return value type");
    }

  forAll(aoi->parameters,
    {
      type = getType(((CAoiParameter*)item)->type);
      if (type && !type->isAllowedParamType())
        semanticError(item->context, "not an allowed parameter type");
    }
  );
}

CASTIdentifier *CBEOperation::buildIdentifier()

{
  if (!aoi->name)
    return NULL;
  
  CASTIdentifier *id = buildClassIdentifier();
  id->addPostfix("_");
  id->addPostfix(aoi->name);

  return id; 
}

CASTIdentifier *CBEOperation::buildClassIdentifier()

{
  return getInterface(aoi->parent)->buildIdentifier();
}  

CASTStatement *CBEOperation::buildClientHeader()

{
  dprintln("*** Building client header for %s", aoi->name);

  assert(connection);

  forAll(aoi->exceptions,
    if (!((CAoiExceptionType*)((CAoiRef*)item)->ref)->members->isEmpty())
      warning("Not implemented: Exceptions with members")
  );

  /* Build a list of all parameters, including the implicit ones */
  
  CASTDeclaration *parameters = NULL;

  bool hasImplicitHandle = false;
  forAll(aoi->parameters, 
    if (getParameter(item)->getFirstMemberWithProperty("handle")) 
      hasImplicitHandle = true
  );

  if (!hasImplicitHandle)
    addTo(parameters, new CASTDeclaration(
      new CASTTypeSpecifier(buildClassIdentifier()),
      knitDeclarator("_service"))
    );
    
  forAll(aoi->parameters, addTo(parameters, getParameter(item)->buildDeclaration()));

  addTo(parameters, new CASTDeclaration(
    new CASTTypeSpecifier("CORBA_Environment"),
    knitIndirectDeclarator("_env"))
  );

  dprintln(" +  Declaring transfer buffer");

  /* Declare local variables and the transfer buffer */

  CASTStatement *declStatements = NULL;
  if (!(aoi->flags&FLAG_ONEWAY))
    addTo(declStatements, new CASTDeclarationStatement(
      msFactory->getMachineWordType()->buildDeclaration(
        new CASTDeclarator(
          new CASTIdentifier("_exception"))))
    );

  CBEVarSource *varSource = new CBEVarSource();
  forAllOps(marshalOps, item->buildClientDeclarations(varSource));
  if (returnOp)
    returnOp->buildClientDeclarations(varSource);
  addTo(declStatements, varSource->declareAll());

  dprintln(" +  Declaring client variables");
  
  addTo(declStatements, connection->buildClientLocalVars(buildIdentifier()));
  if (!aoi->returnType->isVoid())
    {
      CASTDeclarator *rv = new CASTDeclarator(new CASTIdentifier("__retval"));
      rv->addIndir(getType(aoi->returnType)->getArgIndirLevel(RETVAL));
      CASTDeclaration *dclr = getType(aoi->returnType)->buildDeclaration(rv);
      addTo(declStatements, new CASTDeclarationStatement(dclr));
    }

  /* Translate all marshal operations (they were collected during 
     the marshal phase) */

  dprintln(" +  Translating marshal operartions");

  CASTStatement *marshalStatements = NULL;  
  forAllOps(marshalOps, addTo(marshalStatements, item->buildClientMarshal()));
  prependStatement(marshalStatements, connection->buildClientInit());
  if (marshalStatements)
    prependStatement(marshalStatements, new CASTBlockComment("marshal"));

  /* Translate all unmarshal operations (see above) */

  dprintln(" +  Translating unmarshal operations");

  CASTStatement *setExceptionStatements = new CASTIfStatement(
    new CASTBinaryOp("!=",
      new CASTIdentifier("_env"),
      new CASTIntegerConstant(0)),
    connection->buildClientResultAssignment(
      new CASTIdentifier("_env"),
      (aoi->flags&FLAG_ONEWAY) ? (CASTExpression*)new CASTIntegerConstant(0) : (CASTExpression*)new CASTIdentifier("_exception"))
  );

  CASTStatement *unmarshalStatements = NULL;  
  forAllOps(marshalOps, addTo(unmarshalStatements, item->buildClientUnmarshal()));
  addTo(unmarshalStatements, connection->buildClientFinish());
  
  CASTStatement *getStatusStatements = NULL;
  if (!(aoi->flags&FLAG_ONEWAY))
    addTo(getStatusStatements, connection->assignFCCDataFromBuffer(CHANNEL_OUT, 0, new CASTIdentifier("_exception")));

  if (unmarshalStatements)
    {
      prependStatement(unmarshalStatements, new CASTBlockComment("unmarshal"));
      unmarshalStatements = new CASTIfStatement(
        new CASTFunctionOp(
          new CASTIdentifier("IDL4_EXPECT_TRUE"),
          new CASTBinaryOp("&&",
            new CASTBrackets(
              connection->buildClientCallSucceeded()),
            new CASTUnaryOp("!",
              new CASTIdentifier("_exception")))),
        new CASTCompoundStatement(
          unmarshalStatements)
      );
    }   

  /* Return normally */

  CASTStatement *returnStatements = NULL;
  if (returnOp)
    {
      addTo(returnStatements, returnOp->buildClientUnmarshal());
      addTo(returnStatements, new CASTReturnStatement(new CASTIdentifier("__retval")));
    }

  if (returnStatements)
    prependStatement(returnStatements, new CASTBlockComment("return normally"));

  /* Construct the IPC invocation. Note that both marshal and unmarshal
     operations have to be constructed before this point, because the
     CMSConnection needs to know what will go into the registers! */

  CAoiInterface *parent = (CAoiInterface*)aoi->parent;
  int fid = getID();
  
  assert(fid!=UNKNOWN);
  assert(parent);
  assert(parent->iid!=UNKNOWN);
  
  CASTExpression *pathToHandle = new CASTIdentifier("_service");
  forAll(aoi->parameters, 
    {
      CBEParameter *param = getParameter(item);
      if (param->getFirstMemberWithProperty("handle"))
        {
          pathToHandle = param->buildIdentifier();
          for (int i=0;i<param->getArgIndirLevel();i++)
            pathToHandle = new CASTUnaryOp("*", pathToHandle);
            
          while (param!=param->getFirstMemberWithProperty("handle"))
            {
              param = param->getFirstMemberWithProperty("handle");
              pathToHandle = new CASTBinaryOp(".", pathToHandle, param->buildIdentifier());
            }
        }
    }
  );
  
  CASTStatement *invocationStatements = connection->buildClientCall(
    pathToHandle,
    new CASTIdentifier("_env")
  );
  
  if (globals.pre_call_code)
    prependStatement(invocationStatements, new CASTExpressionStatement(
      new CASTIdentifier(globals.pre_call_code))
    );
  if (globals.post_call_code)
    addTo(invocationStatements, new CASTExpressionStatement(
      new CASTIdentifier(globals.post_call_code))
    );
  
  prependStatement(invocationStatements, new CASTBlockComment("invoke IPC"));

  /* Insert all those statements into the stub */

  CASTStatement *statements = NULL;
  addWithTrailingSpacerTo(statements, declStatements);
  addWithTrailingSpacerTo(statements, marshalStatements);
  addTo(statements, invocationStatements);
  addWithLeadingSpacerTo(statements, getStatusStatements);
  addWithLeadingSpacerTo(statements, unmarshalStatements);
  addWithLeadingSpacerTo(statements, setExceptionStatements);
  addWithLeadingSpacerTo(statements, returnStatements);

  /* From the pieces collected above, build the stub declaration */

  CASTCompoundStatement *compound = new CASTCompoundStatement(statements);

  CASTDeclarator *declarator = new CASTDeclarator(buildIdentifier(), parameters);
  declarator->addIndir(getType(aoi->returnType)->getArgIndirLevel(RETVAL));
  CASTDeclaration *declaration = getType(aoi->returnType)->buildDeclaration(declarator, compound);
  
  declaration->addSpecifier(new CASTStorageClassSpecifier("inline"));
  declaration->addSpecifier(new CASTStorageClassSpecifier("static"));

  /* Build a comment with the transfer contract */
  
  enableStringMode(contractBuffer, sizeof(contractBuffer)-1);
  connection->dump();
  disableStringMode();
 
  /* Return the whole thing */
  
  CASTIdentifier *defName = buildIdentifier();
  defName->addPrefix("_funcdef___");
  
  CASTStatement *funcDef = NULL;
  addTo(funcDef, new CASTPreprocDefine(defName, ANONYMOUS));
  addTo(funcDef, new CASTDeclarationStatement(declaration));
  
  CASTStatement *result = NULL;
  if (contractBuffer[0])
    addTo(result, new CASTComment(mprintf(contractBuffer)));
  
  addTo(result, new CASTPreprocConditional(
    new CASTUnaryOp("!", new CASTFunctionOp(new CASTIdentifier("defined"), defName->clone())), funcDef)
  );

  addTo(result, new CASTSpacer());

  dprintln("Completed client header for %s", aoi->name);
 
  return result;
}

void CBEOperation::marshal(CMSService *service)

{
  int exceptionCount = 0;

  assert(!connection);
  assert(!marshalOps);

  dprintln("Marshalling %s::%s", ((CAoiScope*)aoi->parent)->scopedName, aoi->name);

  forAll(aoi->exceptions, exceptionCount++);

  CAoiInterface *parent = (CAoiInterface*)aoi->parent;
  assert(getID()!=UNKNOWN && parent->iid != UNKNOWN);

  this->connection = service->getConnection(2 + exceptionCount, getID(), (aoi->flags & FLAG_KERNELMSG) ? IID_KERNEL : parent->iid);
  if (aoi->flags&FLAG_ONEWAY)
    this->connection->setOption(OPTION_ONEWAY);
  if (aoi->flags&FLAG_LOCAL)
    this->connection->setOption(OPTION_LOCAL);
  if (globals.flags&FLAG_FASTCALL)
    this->connection->setOption(OPTION_FASTCALL);

  /* Add the exception chunk */
  
  if (!(aoi->flags&FLAG_ONEWAY))
    connection->getFixedCopyChunk(CHANNEL_ALL_OUT, 4, "__eid", msFactory->getMachineWordType());

  /* Marshal all parameters */

  forAll(aoi->parameters, getParameter(item)->marshalled = false);

  forAllBEReverse(getOperation(aoi)->sortedParams, 
    addTo(marshalOps, ((CBEParameter*)item)->buildMarshalOps(connection, OP_PROVIDEBUFFER))
  );

  forAll(aoi->parameters, assert(getParameter(item)->marshalled));

  /* If there is a return value, marshal it */

  if (!aoi->returnType->isVoid())
    returnOp = getType(aoi->returnType)->buildMarshalOps(connection, CHANNEL_MASK(CHANNEL_OUT), new CASTIdentifier("__retval"), "__retval", getType(aoi->returnType), NULL, 0);

  connection->optimize();
}

CASTIdentifier *CBEOperation::buildWrapperName(bool capitalize)

{
  CASTIdentifier *wrapperName = buildIdentifier();
  if (capitalize)
    wrapperName->capitalize();
  wrapperName->addPrefix("IDL4_PUBLISH_");
  
  return wrapperName;
}

CASTStatement *CBEOperation::buildServerHeader()

{
  CASTStatement *result = NULL;

  assert(connection);

  /* Declare the necessary types */

  addWithTrailingSpacerTo(result, 
    connection->buildServerDeclarations(buildIdentifier())
  );

  /* Generate a forward declaration for the implementation function */

  CASTDeclaration *parameters = NULL;
  addTo(parameters, new CASTDeclaration(
    new CASTTypeSpecifier(new CASTIdentifier("CORBA_Object")),
    knitDeclarator("_caller"))
  );
    
  forAll(aoi->parameters, addTo(parameters, getParameter(item)->buildDeclaration()));
  addTo(parameters, new CASTDeclaration(
    new CASTTypeSpecifier(new CASTIdentifier("idl4_server_environment")),
    new CASTDeclarator(new CASTIdentifier("_env"), new CASTPointer(1)))
  );

  CASTIdentifier *implementationName = buildIdentifier();
  implementationName->addPostfix("_implementation");

  CASTDeclarator *declarator = new CASTDeclarator(implementationName, parameters);
  declarator->addIndir(getType(aoi->returnType)->getArgIndirLevel(RETVAL));

  CASTDeclaration *declaration = getType(aoi->returnType)->buildDeclaration(declarator);
  declaration->addSpecifier(new CASTStorageClassSpecifier("inline"));

  addWithTrailingSpacerTo(result, new CASTDeclarationStatement(declaration));

  /* Add a description of the buffer layout to the header file */
  
  enableStringMode(contractBuffer, sizeof(contractBuffer)-1);
  connection->dump();
  disableStringMode();
  addTo(result, new CASTComment(mprintf(contractBuffer)));
  
  /* Create the declaration macro */
  
  addTo(result, new CASTPreprocDefine(
    new CASTFunctionOp(
      buildWrapperName(true),
      new CASTIdentifier("_func")),
    connection->buildServerWrapper(buildIdentifier(), buildServerStub()))
  );  
  
  /* Better support for CPP magic (the Neal macro) */
  
  addWithLeadingSpacerTo(result, new CASTPreprocDefine(
    buildWrapperName(false),
    buildWrapperName(true))
  );
     
  /* If this is not a oneway function, generate a reply stub */

  if (!(aoi->flags&FLAG_ONEWAY))
    {
      CASTIdentifier *replyStubName = buildIdentifier();
      replyStubName->addPostfix("_reply");

      connection->reset();

      CASTDeclaration *replyParameters = NULL;
      addTo(replyParameters, new CASTDeclaration(
        new CASTTypeSpecifier(new CASTIdentifier("CORBA_Object")),
        knitDeclarator("_client"))
      );
    
      forAll(aoi->parameters, 
        if (getParameter(item)->isSentFromServerToClient())
          addTo(replyParameters, getParameter(item)->buildDeclaration())
      );
  
      if (!aoi->returnType->isVoid())
        addTo(replyParameters, getType(aoi->returnType)->buildDeclaration(
          new CASTDeclarator(
            new CASTIdentifier("__retval")))
        );

      CASTDeclarationSpecifier *spec = NULL;
      addTo(spec, new CASTStorageClassSpecifier("static"));
      addTo(spec, new CASTStorageClassSpecifier("inline"));
      addTo(spec, new CASTTypeSpecifier(new CASTIdentifier("void")));
      
      addTo(result, new CASTSpacer());
      addWithTrailingSpacerTo(result, new CASTDeclarationStatement(
        new CASTDeclaration(
          spec,
          new CASTDeclarator(
            replyStubName,
            replyParameters),
          buildServerReplyStub()))
      );   
    }

  return result;
}  

CASTCompoundStatement *CBEOperation::buildServerReplyStub()

{
  CASTStatement *result = NULL;
  
  connection->reset();
  
  CBEVarSource *varSource = new CBEVarSource();
  CASTStatement *replyMarshalStatements = NULL;
  forAllOps(marshalOps, 
    {
      item->buildServerReplyDeclarations(varSource);
      addTo(replyMarshalStatements, item->buildServerReplyMarshal());
    }
  );
  if (returnOp)
    {
      returnOp->buildServerReplyDeclarations(varSource);
      addTo(replyMarshalStatements, returnOp->buildServerReplyMarshal());
    }
  if (replyMarshalStatements)
    prependStatement(replyMarshalStatements, new CASTBlockComment("marshal reply"));
    
  addTo(result, varSource->declareAll());
  addTo(result, connection->buildServerReplyDeclarations(buildIdentifier()));
  addWithLeadingSpacerTo(result, replyMarshalStatements);
  addWithLeadingSpacerTo(result, new CASTBlockComment("send message"));
  addTo(result, connection->buildServerReply());
  
  return new CASTCompoundStatement(result);
}

CASTCompoundStatement *CBEOperation::buildServerStub()

{
  /* Translate all marshal operations (they were collected during 
     the marshal phase) */

  assert(connection);

  dprintln("Building server stub for %s", aoi->name);

  CASTStatement *declStatements = NULL;
  CASTStatement *unmarshalStatements = NULL;  
  CBEVarSource *varSource = new CBEVarSource();
  
  forAllOps(marshalOps, 
    {
      item->buildServerDeclarations(varSource);
      addTo(unmarshalStatements, item->buildServerUnmarshal());
    }
  );
  if (returnOp)
    returnOp->buildServerDeclarations(varSource);
  addTo(declStatements, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTTypeSpecifier(new CASTIdentifier("idl4_server_environment")),
      new CASTDeclarator(new CASTIdentifier("_env"))))
  );
       
  if (unmarshalStatements)
    prependStatement(unmarshalStatements, new CASTBlockComment("unmarshal"));

  /************************************************************************/

  CASTStatement *positionCheckStatements = NULL;

  if (debug_mode & DEBUG_TESTSUITE)
    {
      positionCheckStatements = connection->buildServerTestStructural();
      if (positionCheckStatements)
        prependStatement(positionCheckStatements, new CASTBlockComment("position check"));
    }

  /************************************************************************/

  CASTStatement *envInitStatements = NULL;
  
  addTo(envInitStatements, new CASTExpressionStatement(
    new CASTBinaryOp("=",
      new CASTBinaryOp(".",
        new CASTIdentifier("_env"),
        new CASTIdentifier("_action")),
      new CASTIntegerConstant(0)))
  );  

  /************************************************************************/
  
  CASTStatement *invocationStatements = NULL;
  CASTStatement *debugOutStatements = NULL;
  CASTStatement *debugInStatements = NULL;
  CASTStatement *itransStatements = NULL;
  CASTStatement *otransStatements = NULL;
  CASTExpression *arguments = NULL;
  bool transUsed = false;
  
  addTo(arguments, connection->buildServerCallerID());
  forAll(aoi->parameters,
    {
      CAoiParameter *thisParam = (CAoiParameter*)item;
      CBEMarshalOp *thisOp = marshalOps;
      bool found = false;
      
      assert(thisOp); // we seem to have parameters, after all
      do {
           if (thisOp->isResponsibleFor(getParameter(thisParam)))
             { 
               found = true;
               break;
             }
           thisOp = thisOp->next;
         } while (thisOp!=marshalOps);
      if (!found)
        panic("Marshal operation not found for parameter: %s", thisParam->name);
      
      CASTExpression *thisArg = thisOp->buildServerArg(getParameter(thisParam));
      assert(thisArg);
      
      if ((debug_mode & DEBUG_VISUAL) && (thisParam->flags & FLAG_IN))
        addTo(debugInStatements,
          getParameter(thisParam)->buildTestDisplayStmt(buildTestKeyPrefix(), varSource, thisArg->clone())
        );
        
      if ((debug_mode & DEBUG_VISUAL) && (thisParam->flags & FLAG_OUT))
        addTo(debugOutStatements,
          getParameter(thisParam)->buildTestDisplayStmt(buildTestKeyPrefix(), varSource, thisArg->clone())
        );
          
      for (int i=0;i<getParameter(thisParam)->getArgIndirLevel();i++)
        thisArg = new CASTUnaryOp("&", thisArg);
        
      addTo(arguments, thisArg);
      
      CAoiProperty *itrans = thisParam->getProperty("itrans");
      CAoiProperty *otrans = thisParam->getProperty("otrans");

      if (itrans)
        {
          assert(itrans->refValue);
          CASTStatement *itransInvocation = new CASTExpressionStatement(
            new CASTFunctionOp(
              new CASTIdentifier(itrans->refValue),
              knitExprList(
                connection->buildServerCallerID(),
                thisArg->clone(),
                new CASTUnaryOp("&",
                  new CASTIdentifier("_env"))
                ))
          );
          
          if (transUsed)
            itransInvocation = new CASTIfStatement(
              new CASTFunctionOp(
                new CASTIdentifier("IDL4_EXPECT_TRUE"),
                new CASTBinaryOp("==",
                  new CASTBinaryOp(".",
                    new CASTIdentifier("_env"),
                    new CASTIdentifier("_action")),
                  new CASTIntegerConstant(0))),
              itransInvocation
            );
            
          addTo(itransStatements, itransInvocation);
          transUsed = true;
        }  

      if (otrans)
        {
          assert(otrans->refValue);
          CASTStatement *otransInvocation = new CASTExpressionStatement(
            new CASTFunctionOp(
              new CASTIdentifier(otrans->refValue),
              knitExprList(
                connection->buildServerCallerID(),
                thisArg->clone(),
                new CASTUnaryOp("&",
                  new CASTIdentifier("_env"))
                ))
          );
          
          otransInvocation = new CASTIfStatement(
            new CASTFunctionOp(
              new CASTIdentifier("IDL4_EXPECT_TRUE"),
              new CASTBinaryOp("==",
                new CASTBinaryOp(".",
                  new CASTIdentifier("_env"),
                  new CASTIdentifier("_action")),
                new CASTIntegerConstant(0))),
            otransInvocation
          );
            
          addTo(otransStatements, otransInvocation);
        }  
    }
  );
  addTo(arguments, new CASTUnaryOp("&", new CASTIdentifier("_env")));

  if (itransStatements)
    prependStatement(itransStatements, new CASTBlockComment("apply input translations"));
    
  if (otransStatements)
    prependStatement(otransStatements, new CASTBlockComment("apply output translations"));

  if (globals.pre_call_code)
    addTo(itransStatements, new CASTExpressionStatement(
      new CASTIdentifier(globals.pre_call_code))
    );
  if (globals.post_call_code)
    prependStatement(otransStatements, new CASTExpressionStatement(
      new CASTIdentifier(globals.post_call_code))
    );

  CASTExpression *invocation = new CASTFunctionOp(new CASTIdentifier("_func"), arguments);
  if (!aoi->returnType->isVoid())
    {
      invocation = new CASTBinaryOp("=",
        returnOp->buildServerArg(NULL),
        invocation
      );  
      
      char nameStr[100];
      enableStringMode(nameStr, sizeof(nameStr));
      getType(aoi->returnType)->buildDeclaration(new CASTDeclarator(new CASTIdentifier("__retval")))->write();
      disableStringMode();
      
      addTo(debugOutStatements,
        new CASTExpressionStatement(
          new CASTFunctionOp(
            new CASTIdentifier("printf"),
            new CASTStringConstant(false, mprintf("out %s = ", nameStr))))
      );
      
      addTo(debugOutStatements,
        getType(aoi->returnType)->buildTestDisplayStmt(
          new CASTIdentifier("__retval"),
          new CASTIdentifier("__retval"),
          varSource, RETVAL, 
          returnOp->buildServerArg(NULL))
      );

      addTo(debugOutStatements,
        new CASTExpressionStatement(
          new CASTFunctionOp(
            new CASTIdentifier("printf"),
            new CASTStringConstant(false, "\\n")))
      );
    }

  addTo(invocationStatements, new CASTBlockComment(mprintf("invoke service")));  
  addTo(declStatements, varSource->declareAll());
  
  if (debug_mode&DEBUG_VISUAL)
    {
      addTo(invocationStatements, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("printf"),
          knitExprList(
            new CASTStringConstant(false, mprintf("*** Servicing call from %%X to %s::%s\\n", ((CAoiScope*)aoi->parent)->scopedName, aoi->name)),
            new CASTFunctionOp(
              new CASTIdentifier("idl4_raw_fpage"),
              connection->buildServerCallerID()
            ))))
      );
      addTo(invocationStatements, debugInStatements);
      addTo(invocationStatements, new CASTSpacer());
    }

  CASTStatement *invocationTemp = new CASTExpressionStatement(invocation);
  if (transUsed)
    invocationTemp = new CASTIfStatement(
      new CASTFunctionOp(
        new CASTIdentifier("IDL4_EXPECT_TRUE"),
          new CASTBinaryOp("==",
          new CASTBinaryOp(".",
            new CASTIdentifier("_env"),
            new CASTIdentifier("_action")),
          new CASTIntegerConstant(0))),
      invocationTemp
    );
    
  addTo(invocationStatements, invocationTemp);
  
  if (debug_mode&DEBUG_VISUAL)
    {
      addTo(invocationStatements, new CASTSpacer());
      addTo(invocationStatements, new CASTExpressionStatement(
        new CASTFunctionOp(
          new CASTIdentifier("printf"),
          knitExprList(
            new CASTStringConstant(false, "--- Return code is %d\\n"),
              new CASTBinaryOp(".",
                new CASTIdentifier("_env"),
                new CASTIdentifier("_action"))
            )))
      );
      addTo(invocationStatements, debugOutStatements);
    }
  
  addTo(invocationStatements, connection->buildServerMarshalInit());

  /************************************************************************/

  CASTStatement *marshalStatements = NULL;
  if (!(aoi->flags & FLAG_ONEWAY))
    {
      forAllOps(marshalOps, addTo(marshalStatements, item->buildServerMarshal()));
      if (!aoi->returnType->isVoid())
        addTo(marshalStatements, returnOp->buildServerMarshal());
      if (marshalStatements)
        prependStatement(marshalStatements, new CASTBlockComment("marshal"));
    
      addWithLeadingSpacerTo(marshalStatements, new CASTBlockComment("jump back"));
      addTo(marshalStatements, connection->buildServerBackjump(CHANNEL_OUT, new CASTIdentifier("_env")));
  
      marshalStatements = new CASTIfStatement(
        new CASTFunctionOp(
          new CASTIdentifier("IDL4_EXPECT_TRUE"),
          new CASTBinaryOp("==",
            new CASTBinaryOp(".",
              new CASTIdentifier("_env"),
              new CASTIdentifier("_action")),
            new CASTIntegerConstant(0))),
        new CASTCompoundStatement(
          marshalStatements)
      );
    }

  /************************************************************************/

  CASTStatement *exceptionStatements = NULL;
  int channelID = CHANNEL_OUT + 1;
  
  forAll(aoi->exceptions,
    {
      CBEExceptionType *bExc = getException(((CAoiRef*)item)->ref);
      CASTStatement *backjump;

      connection->reset();
      backjump = connection->buildServerBackjump(channelID++, new CASTIdentifier("_env"));
      if (backjump->next != backjump)
        backjump = new CASTCompoundStatement(backjump);
      
      CASTStatement *newBackjump = new CASTIfStatement(
        new CASTBinaryOp("==",
          new CASTBinaryOp(".",
            new CASTIdentifier("_env"),
            new CASTIdentifier("_action")),
          new CASTBinaryOp("+",
            new CASTIdentifier("CORBA_USER_EXCEPTION"),
            new CASTBinaryOp("<<",
              bExc->buildID(),
              new CASTIntegerConstant(8)))),
        backjump
      );
      addWithLeadingSpacerTo(exceptionStatements, newBackjump);
    }
  );

  if (exceptionStatements)
    prependStatement(exceptionStatements, new CASTBlockComment("handle exceptions"));

  /************************************************************************/

  /* This must be done last, because the backend might need to allocate
     additional buffers during marshalling */
     
  addTo(declStatements, connection->buildServerLocalVars(buildIdentifier()));

  /************************************************************************/

  CASTStatement *stub = NULL;
  addWithTrailingSpacerTo(stub, declStatements);
  addWithTrailingSpacerTo(stub, envInitStatements);
  addWithTrailingSpacerTo(stub, positionCheckStatements);
  addWithTrailingSpacerTo(stub, unmarshalStatements);
  addWithTrailingSpacerTo(stub, itransStatements);
  addTo(stub, invocationStatements);
  addWithLeadingSpacerTo(stub, otransStatements);
  addWithLeadingSpacerTo(stub, marshalStatements);
  addWithLeadingSpacerTo(stub, exceptionStatements);
  addWithLeadingSpacerTo(stub, connection->buildServerAbort());

  return new CASTCompoundStatement(stub);
}

CASTStatement *CBEOperation::buildServerTemplate()

{
  CASTStatement *result = NULL;
  CASTDeclaration *parameters = NULL;
  
  assert(connection);
  
  dprintln("*** Building server template for %s", aoi->name);

  addTo(parameters, new CASTDeclaration(
    new CASTTypeSpecifier(new CASTIdentifier("CORBA_Object")),
    knitDeclarator("_caller"))
  );
    
  forAll(aoi->parameters, addTo(parameters, getParameter(item)->buildDeclaration()));
  addTo(parameters, new CASTDeclaration(
    new CASTTypeSpecifier(new CASTIdentifier("idl4_server_environment")),
    new CASTDeclarator(new CASTIdentifier("_env"), new CASTPointer(1)))
  );

  CASTIdentifier *implementationName = buildIdentifier();
  implementationName->addPostfix("_implementation");

  CBEVarSource *varSource = new CBEVarSource();
  CASTStatement *localVars = NULL;
  CASTStatement *statements = NULL;

  if (debug_mode&DEBUG_TESTSUITE)
    addTo(statements, buildTestClientCode(varSource));
    else addTo(statements, new CASTBlockComment(mprintf("implementation of %s::%s", ((CAoiInterface*)aoi->parent)->scopedName, aoi->name)));

  dprintln(" +  Marshalling return value");
  
  addTo(localVars, varSource->declareAll());
  if (!aoi->returnType->isVoid())
    {
      CASTDeclarator *rv = new CASTDeclarator(new CASTIdentifier("__retval"), NULL, NULL, getType(aoi->returnType)->buildDefaultValue());
      rv->addIndir(getType(aoi->returnType)->getArgIndirLevel(RETVAL));
      addTo(localVars, new CASTDeclarationStatement(
        getType(aoi->returnType)->buildDeclaration(rv))
      );

      addTo(statements, new CASTReturnStatement(
        new CASTIdentifier("__retval")));
    } else addTo(statements, new CASTReturnStatement());

  CASTStatement *compound = NULL;
  addWithTrailingSpacerTo(compound, localVars);
  addTo(compound, statements);
  
  CASTDeclarator *declarator = new CASTDeclarator(implementationName, parameters);
  declarator->addIndir(getType(aoi->returnType)->getArgIndirLevel(RETVAL));
  CASTDeclaration *declaration = getType(aoi->returnType)->buildDeclaration(declarator, new CASTCompoundStatement(compound));
  declaration->addSpecifier(new CASTStorageClassSpecifier(mprintf("IDL4_INLINE")));
  
  addWithTrailingSpacerTo(result, new CASTDeclarationStatement(declaration));

  addWithTrailingSpacerTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      buildWrapperName(true),
      implementationName->clone()))
  );
  
  return result;
}  

CASTIdentifier *CBEInheritedOperation::buildClassIdentifier()

{
  return getInterface(actualParent)->buildIdentifier();
}  

CASTIdentifier *CBEOperation::buildOriginalClassIdentifier()

{
  return getInterface(aoi->parent)->buildIdentifier();
}

CBEInheritedOperation::CBEInheritedOperation(CAoiOperation *aoi, CAoiInterface *actualParent) : CBEOperation(aoi)

{
  this->actualParent = actualParent;
}

CASTStatement *CBEInheritedOperation::buildClientHeader()

{
  CASTStatement *result = new CASTComment(mprintf("Inherited from %s", ((CAoiInterface*)aoi->parent)->scopedName));
  addTo(result, CBEOperation::buildClientHeader());
  return result;
}

CASTStatement *CBEInheritedOperation::buildServerHeader()

{
  CASTStatement *result = new CASTComment(mprintf("Inherited from %s", ((CAoiInterface*)aoi->parent)->scopedName));
  addTo(result, CBEOperation::buildServerHeader());
  return result;
}

void CBEOperation::assignID(int id)

{
  this->id = id;
}

int CBEOperation::getID()

{
  if (aoi->fid!=UNKNOWN) return aoi->fid; 
  
  return id;
}

int CBEInheritedOperation::getID()

{
  return getOperation(aoi)->getID();
}
