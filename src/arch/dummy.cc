#include "arch/dummy.h"

/***************************** ALLOCATION PHASE *****************************/
/*   The backend analyzes the parameters and allocates the chunks it needs  */
/****************************************************************************/

CMSConnectionDummy::CMSConnectionDummy(CMSService *service, int fid, int iid) : CMSConnection(service, 0, fid, iid)

{
  nextChunk = 0;
}

void CMSConnectionDummy::setOption(int option)

{
  /* Called:   By the backend, when the requested connection needs special
               properties. One example of this is OPTION_ONEWAY, which is
               set for oneway functions to indicate that no reply message
               must be generated
     Purpose:  Update the internal state
     Returns:  Nothing */
}

ChunkID CMSConnectionDummy::getFixedCopyChunk(int channels, int size, const char *name, CBEType *type)

{
  /* Called:   When the backend wants to allocate a FCC of the specified size
     Purpose:  Allocate sufficient space in the buffer, e.g. in a memory message
     Returns:  An identifier. This will later be supplied by the backend when
               accessing the FCC, e.g. by calling assignFCCDataToBuffer() */

  return nextChunk++;
}

ChunkID CMSConnectionDummy::getVariableCopyChunk(int channels, int minSize, int typSize, int maxSize, const char *name, CBEType *type)

{
  /* Called:   When the backend wants to allocate a VCC of the specified
               minimum, typical, and maximum size
     Purpose:  Allocate sufficient space in the buffer, e.g. an indirect string
     Returns:  An identifier. This will later be supplied bhy the backend when
               accessing the VCC, e.g. by calling assignVCCSizeAndDataToBuffer() */
               
  return nextChunk++;
}
  
ChunkID CMSConnectionDummy::getMapChunk(int channels, int size, const char *name)

{
  /* Called:   When the backend wants to allocate a FMC of the specified size
     Purpose:  Allocate sufficient space in the buffer, e.g. MapItems
     Returns:  An identifier. This will later be supplied bhy the backend when
               accessing the FMC, e.g. by calling assignFMCFpageToBuffer() */

  return nextChunk++;
}

void CMSConnectionDummy::optimize()

{
  /* Called:   When all chunks have been added to the connection
     Purpose:  Rearrange the chunks and create an efficient message layout
     Returns:  Nothing */
}

void CMSServiceDummy::finalize()

{
  /* Called:   When all connections have been added to the buffer
     Purpose:  Make final changes to the buffer layout, e.g. calculate
               the maximum buffer size required
     Returns:  Nothing */
}

void CMSConnectionDummy::reset()

{
  /* Called:   Before the backend starts to generate a new stub
     Purpose:  Reset internal variables, e.g. empty register buffers
     Returns:  Nothing */
}

/******************************* ACCESS PHASE *******************************/
/*   For marshaling/unmarshaling, the backend needs to access the buffer    */
/****************************************************************************/

CASTStatement *CMSConnectionDummy::assignFCCDataToBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue)

{
  /* Called:   By the backend (client or server side)
     Purpose:  Generate a statement that writes the supplied rvalue to the
               buffer. This action may be delayed (e.g. for register values);
               in this case, the rvalue must be stored for later use
     Returns:  A chain of statements, e.g. an assignment */

  return new CASTExpressionStatement(
    new CASTBinaryOp("=", 
      new CASTBinaryOp(".",
        new CASTBinaryOp(".",
          new CASTIdentifier("_buffer"),
          new CASTIdentifier(mprintf("channel%d", channel))),
        new CASTIdentifier(mprintf("fcc_chunk%d", chunkID))),
      rvalue)
  );
}

CASTStatement *CMSConnectionDummy::assignFCCDataFromBuffer(int channel, ChunkID chunkID, CASTExpression *lvalue)

{
  /* Called:   By the backend (client or server side)
     Purpose:  Generate a statement that reads the designated chunk from the
               buffer and stores its value to the specified lvalue. 
               This action may be delayed (e.g. for register values);
               in this case, the lvalue must be stored for later use
     Returns:  A chain of statements, e.g. an assignment */

  return new CASTExpressionStatement(
    new CASTBinaryOp("=", 
      lvalue,
      new CASTBinaryOp(".",
        new CASTBinaryOp(".",
          new CASTIdentifier("_buffer"),
          new CASTIdentifier(mprintf("channel%d", channel))),
        new CASTIdentifier(mprintf("fcc_chunk%d", chunkID))))
  );
}

CASTExpression *CMSConnectionDummy::buildFCCDataSourceExpr(int channel, ChunkID chunk)

{
  /* Called:   By the backend (client or server side)
     Purpose:  Produce an expression that can be used to read the value of
               the chunk directly from the source buffer (i.e. the buffer that
               is used for sending)
     Returns:  An expression */
     
  return new CASTBinaryOp(".",
    new CASTBinaryOp(".",
      new CASTIdentifier("_buffer"),
      new CASTIdentifier(mprintf("channel%d", channel))),
    new CASTIdentifier(mprintf("fcc_chunk%d", chunk))
  );
}

CASTExpression *CMSConnectionDummy::buildFCCDataTargetExpr(int channel, ChunkID chunk)

{
  /* Called:   By the backend (client or server side)
     Purpose:  Produce an expression that can be used to read the value of
               the chunk directly from the target buffer (i.e. the buffer that
               is used for receiving)
     Returns:  An expression */
     
  return new CASTBinaryOp(".",
    new CASTBinaryOp(".",
      new CASTIdentifier("_buffer"),
      new CASTIdentifier(mprintf("channel%d", channel))),
    new CASTIdentifier(mprintf("fcc_chunk%d", chunk))
  );
}

CASTStatement *CMSConnectionDummy::assignVCCSizeAndDataToBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue, CASTExpression *size)

{
  /* Called:   By the backend (client or server side)
     Purpose:  Produce a sequence of statements that store a VCC and its size
               to the buffer
     Returns:  One or multiple statements */
     
  return new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("memmove"),
      knitExprList(
        new CASTBinaryOp(".",
          new CASTBinaryOp(".",
            new CASTIdentifier("_buffer"),
            new CASTIdentifier(mprintf("channel%d", channel))),
          new CASTIdentifier(mprintf("vcc_chunk%d", chunkID))),
        rvalue, 
        size))
  );    
}

CASTStatement *CMSConnectionDummy::assignVCCDataFromBuffer(int channel, ChunkID chunkID, CASTExpression *lvalue)

{
  /* Called:   By the backend (client or server side)
     Purpose:  Produce a sequence of statements that assign the address of
               a VCC to the specified lvalue. Useful e.g. on the server
               side, when the backend does not want to copy the VCC to a 
               buffer of its own
     Returns:  One or multiple statements */
     
  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      lvalue, 
      new CASTUnaryOp("&",
        new CASTBinaryOp(".",
          new CASTBinaryOp(".",
            new CASTBinaryOp(".",
              new CASTIdentifier("_buffer"),
              new CASTIdentifier(mprintf("channel%d", channel))),
            new CASTIdentifier(mprintf("vcc_chunk%d", chunkID))),
          new CASTIdentifier("data"))))
  );
}

CASTStatement *CMSConnectionDummy::assignVCCSizeFromBuffer(int channel, ChunkID chunkID, CASTExpression *lvalue)

{
  /* Called:   By the backend (client or server side)
     Purpose:  Produce a sequence of statements that assign the size of
               a VCC to the specified lvalue
     Returns:  One or multiple statements */
     
  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      lvalue, 
      new CASTUnaryOp("&",
        new CASTBinaryOp(".",
          new CASTBinaryOp(".",
            new CASTBinaryOp(".",
              new CASTIdentifier("_buffer"),
              new CASTIdentifier(mprintf("channel%d", channel))),
            new CASTIdentifier(mprintf("vcc_chunk%d", chunkID))),
          new CASTIdentifier("size"))))
  );
}

CASTStatement *CMSConnectionDummy::assignFMCFpageToBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue)

{
  /* Called:   By the backend (client or server side)
     Purpose:  Produce a sequence of statements that store a FMC to the buffer
     Returns:  One or multiple statements */

  return new CASTExpressionStatement(
    new CASTBinaryOp("=", 
      new CASTBinaryOp(".",
        new CASTBinaryOp(".",
          new CASTIdentifier("_buffer"),
          new CASTIdentifier(mprintf("channel%d", channel))),
        new CASTIdentifier(mprintf("fmc_chunk%d", chunkID))),
      rvalue)
  );
}

CASTStatement *CMSConnectionDummy::assignFMCFpageFromBuffer(int channel, ChunkID chunkID, CASTExpression *rvalue)

{
  /* Called:   By the backend (client or server side)
     Purpose:  Produce a sequence of statements that load a FMC from the buffer
     Returns:  One or multiple statements */

  return new CASTExpressionStatement(
    new CASTBinaryOp("=", 
      rvalue,
      new CASTBinaryOp(".",
        new CASTBinaryOp(".",
          new CASTIdentifier("_buffer"),
          new CASTIdentifier(mprintf("channel%d", channel))),
        new CASTIdentifier(mprintf("fmc_chunk%d", chunkID))))
  );
}

CASTExpression *CMSConnectionDummy::buildFMCFpageSourceExpr(int channel, ChunkID chunkID)

{
  /* Called:   By the backend (client or server side)
     Purpose:  Produce an expression that can be used to read the value of
               the chunk directly from the source buffer (i.e. the buffer that
               is used for sending)
     Returns:  An expression */
     
  return new CASTBinaryOp(".",
    new CASTBinaryOp(".",
      new CASTIdentifier("_buffer"),
      new CASTIdentifier(mprintf("channel%d", channel))),
    new CASTIdentifier(mprintf("fmc_chunk%d", chunkID))
  );
}

CASTExpression *CMSConnectionDummy::buildFMCFpageTargetExpr(int channel, ChunkID chunkID)

{
  /* Called:   By the backend (client or server side)
     Purpose:  Produce an expression that can be used to read the value of
               the chunk directly from the target buffer (i.e. the buffer that
               is used for receiving)
     Returns:  An expression */
     
  return new CASTBinaryOp(".",
    new CASTBinaryOp(".",
      new CASTIdentifier("_buffer"),
      new CASTIdentifier(mprintf("channel%d", channel))),
    new CASTIdentifier(mprintf("fmc_chunk%d", chunkID))
  );
}

/********************************* ALL STUBS ********************************/
/*         These functions are used both for client and server stubs        */
/****************************************************************************/

void CMSConnectionDummy::dump()

{
  /* Called:   By the backend (client and server side)
     Purpose:  Create some human-readable description of the message layout,
               which is inserted as a multi-line comment into the code
     Returns:  Nothing */

  println("Dummy connection - %d chunks allocated", nextChunk);
}

/******************************** CLIENT STUB *******************************/
/*      These functions are used by the backend to create client stubs      */
/****************************************************************************/

CASTStatement *CMSConnectionDummy::buildClientLocalVars(CASTIdentifier *key)

{
  /* Called:   By the backend (client side)
     Purpose:  Declare any variables the client stub might need, e.g. counters
     Returns:  A chain of declaration statements */

  CASTStatement *result = NULL;
  addTo(result, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTTypeSpecifier("someType"),
        new CASTDeclarator(
          new CASTIdentifier("_clientLocalVar"))))
  );
  addTo(result, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTTypeSpecifier("bufferType"),
        new CASTDeclarator(
          new CASTIdentifier("_buffer"))))
  );
  
  return result;
}

CASTStatement *CMSConnectionDummy::buildClientInit()

{
  /* Called:   By the backend (client side)
     Purpose:  Initialization and preloading, e.g. pointers or counters
     Returns:  One or multiple statements */

  return new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("clientInit"),
      new CASTIdentifier("_buffer"))
  );
}

CASTStatement *CMSConnectionDummy::buildClientCall(CASTExpression *target, CASTExpression *env)

{
  /* Called:   By the backend (client side)
     Purpose:  Invoke the IPC system call
     Returns:  One or multiple statements */
     
  return new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("ipc"),
      new CASTIdentifier("_buffer"))
  );
}

CASTExpression *CMSConnectionDummy::buildClientCallSucceeded()

{
  /* Called:   By the backend (client side)
     Purpose:  Determine whether the IPC went through
     Returns:  A boolean expression */
     
  return new CASTFunctionOp(
    new CASTIdentifier("callSucceeded")
  );
}

CASTStatement *CMSConnectionDummy::buildClientResultAssignment(CASTExpression *environment, CASTExpression *defaultValue)

{
  /* Called:   By the backend (client side)
     Purpose:  Set the exception code in the environment. If the IPC failed,
               any system exception may be set, otherwise the default value
               should be assigned
     Returns:  A sequence of statements */
     
  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      new CASTBinaryOp(".",
        environment,
        new CASTIdentifier("_major")),
      defaultValue)
  );
}

CASTStatement *CMSConnectionDummy::buildClientFinish()

{
  /* Called:   By the backend (client side)
     Purpose:  Cleanup, e.g. release buffers
     Returns:  One or multiple statements */

  return new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("clientFinish"),
      new CASTIdentifier("_buffer"))
  );
}

CASTStatement *CMSConnectionDummy::provideVCCTargetBuffer(int channel, ChunkID chunkID, CASTExpression *lvalue, CASTExpression *size)

{
  /* Called:   By the backend (client side)
     Purpose:  Produce a sequence of statements that preallocate a buffer
               for an outbound VCC
     Returns:  One or multiple statements */

  return new CASTExpressionStatement(
    new CASTBinaryOp("=", 
      lvalue,
      new CASTFunctionOp(
        new CASTIdentifier("malloc"),
        size))
  );
}

/******************************** SERVER STUB *******************************/
/*      These functions are used by the backend to create server stubs      */
/****************************************************************************/

CASTStatement *CMSServiceDummy::buildServerDeclarations(CASTIdentifier *prefix, int vtableSize, int itableSize, int ktableSize)

{
  /* Called:   By the backend (server side)
     Purpose:  Declare any types or variables that are needed by all stubs 
               of this service, or by the server loop (e.g. vtables)
     Returns:  A chain of declaration statements */

  CASTDeclarationSpecifier *specs = NULL;
  addTo(specs, new CASTStorageClassSpecifier("extern"));
  addTo(specs, new CASTTypeSpecifier("vtableType"));

  prefix->addPostfix("_VTABLE");
  return new CASTDeclarationStatement(
    new CASTDeclaration(
      specs,
        new CASTDeclarator(
          prefix,
          NULL,
          new CASTEmptyConstant()))
  );
}

CASTStatement *CMSConnectionDummy::buildServerDeclarations(CASTIdentifier *key)

{
  /* Called:   By the backend (server side)
     Purpose:  Declare types needed by the server stub (e.g. the buffer type)
     Returns:  One or multiple declaration statements */
     
  CASTIdentifier *typeIdentifier = key->clone();
  typeIdentifier->addPostfix("_buftype");
 
  CASTDeclarationSpecifier *specs = NULL;
  addTo(specs, new CASTStorageClassSpecifier("typedef"));
  addTo(specs, new CASTAggregateSpecifier(
    "struct", 
    ANONYMOUS,
    new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTTypeSpecifier( 
          new CASTIdentifier("int")),
        new CASTDeclarator(
          new CASTIdentifier("bufferElements")))))
  );  
  
  return new CASTDeclarationStatement(
    new CASTDeclaration(
      specs,
      new CASTDeclarator(typeIdentifier))
  );
}

CASTStatement *CMSConnectionDummy::buildServerLocalVars(CASTIdentifier *key)

{
  /* Called:   By the backend (server side)
     Purpose:  Declare any variables the server stub might need, e.g. counters
     Returns:  A chain of declaration statements */

  return new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTTypeSpecifier("someType"),
        new CASTDeclarator(
          new CASTIdentifier("_serverLocalVar")))
  );
}

CASTStatement *CMSConnectionDummy::buildServerMarshalInit()

{
  /* Called:   By the backend (server side)
     Purpose:  Reset the state of the server stub before the output values
               are marshalled
     Returns:  A chain of statements */
     
  return new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("serverMarshalInit"))
  );
}

CASTStatement *CMSConnectionDummy::buildVCCServerPrealloc(int channel, ChunkID chunkID, CASTExpression *lvalue)

{
  /* Called:   By the backend (server side)
     Purpose:  Produce a sequence of statements that allocate a temporary
               buffer for the designated VCC chunk. This is necessary for
               outbound VCCs; we give an empty buffer to the user code,
               where it is filled with data
     Returns:  One or multiple statements */
     
  return new CASTExpressionStatement(
    new CASTBinaryOp("=",
      lvalue,
      new CASTFunctionOp(
        new CASTIdentifier("getTemporaryBuffer")))
  );
}

CASTStatement *CMSConnectionDummy::buildServerBackjump(int channel, CASTExpression *environment)

{
  /* Called:   By the backend (server side)
     Purpose:  Do some cleanup, then return to the server loop
     Returns:  One or multiple statements */

  CASTStatement *result = NULL;
  
  addWithTrailingSpacerTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("doSomeCleanup")))
  );
  addTo(result, new CASTReturnStatement(
    new CASTIdentifier("someResult"))
  );
  
  return result;
}

CASTStatement *CMSConnectionDummy::buildServerAbort()

{
  /* Called:   By the backend (server side)
     Purpose:  Immediately return to the server loop, do not send an reply
     Returns:  One or multiple statements */

  CASTStatement *result = NULL;
  
  return new CASTReturnStatement(
    new CASTIdentifier("noResult")
  );
}

CASTExpression *CMSConnectionDummy::buildServerCallerID()

{
  /* Called:   By the backend (server side)
     Purpose:  Return an expression that evaluates to the ID of the caller,
               e.g. the threadID of the RPC sender
     Returns:  One or multiple statements */

  return new CASTIdentifier("_callerID");
}

CASTStatement *CMSConnectionDummy::buildServerTestStructural()

{
  /* Called:   By the backend (server side, only for the regression test)
     Purpose:  Perform some run-time checks on the buffer. The test suite
               provides a 'panic' function which may be called when an error
               is detected
     Returns:  A chain of statements */

  return new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("checkBufferStructure"),
      new CASTIdentifier("_buffer"))
  );
}

CASTBase *CMSConnectionDummy::buildServerWrapper(CASTIdentifier *key, CASTCompoundStatement *compound)

{
  /* Called:   By the backend (server side), when the stub has been generated
     Purpose:  Wrap the stub, whose compound statement is provided, 
               into a function
     Returns:  Any CAST element, e.g. a declaration */
     
  CBEType *voidType = getType(aoiRoot->lookupSymbol("#void", SYM_TYPE));
  CASTIdentifier *wrapperIdentifier = key->clone();

  wrapperIdentifier->addPrefix("service_");
  return voidType->buildDeclaration(
    new CASTDeclarator(
      wrapperIdentifier,
      new CASTDeclaration(
        new CASTTypeSpecifier(
          new CASTIdentifier("bufferType")),
        new CASTDeclarator(
          new CASTIdentifier("_buffer")))), 
    compound
  );
}

/******************************** REPLY STUB ********************************/
/*      These functions are used by the backend to create reply stubs       */
/****************************************************************************/

CASTStatement *CMSConnectionDummy::buildServerReplyDeclarations(CASTIdentifier *key)

{
  /* Called:   By the backend (server side)
     Purpose:  Declare types needed by the reply stub (e.g. the buffer type)
     Returns:  One or multiple declaration statements */
     
  CASTIdentifier *typeIdentifier = key->clone();
  typeIdentifier->addPostfix("_buftype");
 
  CASTDeclarationSpecifier *specs = NULL;
  addTo(specs, new CASTStorageClassSpecifier("typedef"));
  addTo(specs, new CASTAggregateSpecifier(
    "struct", 
    ANONYMOUS,
    new CASTDeclarationStatement(
      new CASTDeclaration(
        new CASTTypeSpecifier( 
          new CASTIdentifier("int")),
        new CASTDeclarator(
          new CASTIdentifier("replyBufferElements")))))
  );  
  
  return new CASTDeclarationStatement(
    new CASTDeclaration(
      specs,
      new CASTDeclarator(typeIdentifier))
  );
}

CASTStatement *CMSConnectionDummy::buildServerReply()

{
  /* Called:   By the backend (server side)
     Purpose:  Send the reply message to the client, return immediately
     Returns:  One or multiple statements */

  CASTStatement *result = NULL;
  
  addWithTrailingSpacerTo(result, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("sendMessageTo"),
      new CASTIdentifier("_client")))
  );
  
  return result;
}

/******************************** SERVER LOOP *******************************/
/*        These functions are used to create a server loop template         */
/****************************************************************************/

CASTStatement *CMSServiceDummy::buildServerLoop(CASTIdentifier *prefix, CASTExpression *utableRef, CASTExpression *ktableRef, bool useItable, bool hasKernelMessagess)

{
  /* Called:   By the backend (template mode)
     Purpose:  Provide a server loop template. This is intended as a starting
               point for the user and can have only basic features.
               However, it must be functional, as it is used by the regression
               test suite.
     Returns:  A chain of statements */

  CASTStatement *loop = NULL;
  addTo(loop, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("receiveRequest")))
  );
  addTo(loop, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("handleRequest")))
  );
  addTo(loop, new CASTExpressionStatement(
    new CASTFunctionOp(
      new CASTIdentifier("sendReply")))
  );

  return new CASTWhileStatement(
    new CASTIntegerConstant(1),
    new CASTCompoundStatement(loop)
  );
}

CASTStatement *CMSFactoryDummy::buildIncludes()

{
  /* Called:   By the backend
     Purpose:  Include any header files the stubs might need
     Returns:  A chain of statements */

  return new CASTPreprocInclude(new CASTIdentifier(mprintf("%sidl4/dummy.h", globals.system_prefix)));
}

CASTStatement *CMSFactoryDummy::buildTestIncludes()

{
  /* Called:   By the backend
     Purpose:  Include any header files the stubs might need in testing mode
     Returns:  A chain of statements */

  return new CASTPreprocInclude(new CASTIdentifier(mprintf("%sidl4/test/dummy1.h", globals.system_prefix)));
}
