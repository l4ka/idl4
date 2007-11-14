#include "be.h"
#include "ms.h"

#define voidType (getType(aoiRoot->lookupSymbol("#void", SYM_TYPE)))

void CBEInterface::claimIDs(CBEIDSource *source)

{
  CBEScope::claimIDs(source);
  forAll(aoi->superclasses, getScope(((CAoiRef*)item)->ref)->avoidConflicts(this, source));
}  

void CBEInterface::marshal()

{
  CBEScope::marshal();
  if (aoi->flags&FLAG_LOCAL)
    service = msFactory->getLocalService();
    else service = msFactory->getService();
  forAll(aoi->operations, getOperation(item)->marshal(service));
  forAllBE(inheritedOps, ((CBEOperation*)item)->marshal(service));
  service->finalize();
}

void CBEInterface::avoidConflicts(CBEScope *invoker, CBEIDSource *source)

{
  CBEScope::avoidConflicts(invoker, source);
  forAll(aoi->superclasses, getScope(((CAoiRef*)item)->ref)->avoidConflicts(invoker, source));
}

CASTIdentifier *CBEInterface::buildIdentifier()

{
  return knitScopedNameFrom(aoi->scopedName);
}

CASTStatement *CBEInterface::buildTestInvocation()

{
  CASTStatement *result = NULL;
  
  CASTExpression *arguments = NULL;
  addTo(arguments, new CASTIdentifier("server"));
  
  addTo(arguments, new CASTUnaryOp("(unsigned)", buildServerFuncName()));
  
  addTo(result,
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("idl4_thread_init"),
        arguments))
  );

  CASTExpression *serverID = new CASTIdentifier("server");

  if (aoi->flags&FLAG_LOCAL)
    {
      serverID = new CASTFunctionOp(
                   new CASTIdentifier("idl4_gid2lid"),
                   serverID
                 );
    }
    
  forAll(aoi->operations,
    addTo(result,
      new CASTExpressionStatement(
        new CASTFunctionOp(
          getOperation(item)->buildTestFuncName(),
          serverID)))
  );

  forAllBE(inheritedOps,
    addTo(result, 
      new CASTExpressionStatement(
        new CASTFunctionOp(
          ((CBEOperation*)item)->buildTestFuncName(),
          serverID)))
  );
  
  addTo(result,
    new CASTExpressionStatement(
      new CASTFunctionOp(
        new CASTIdentifier("sfree")))
  );
  
  return result;
}

CASTStatement *CBEInterface::buildTitleComment()

{
  return new CASTBlockComment(mprintf("Interface %s", aoi->scopedName));
}

CASTStatement *CBEInterface::buildReferenceDefinition()

{  
  CASTStatement *result = NULL;
  
  CASTDeclarationSpecifier *specifiers = NULL;
  addTo(specifiers, new CASTStorageClassSpecifier("typedef"));
  addTo(specifiers, new CASTTypeSpecifier("CORBA_Object"));
  
  CASTIdentifier *defName = buildIdentifier();
  defName->addPrefix("_objdef___");

  CASTBase *subtree = new CASTPreprocDefine(defName, ANONYMOUS);
  addTo(subtree, new CASTDeclarationStatement(
    new CASTDeclaration(
      specifiers,
      new CASTDeclarator(buildIdentifier())))
  );
  
  addWithTrailingSpacerTo(result, new CASTPreprocConditional(
    new CASTUnaryOp("!", 
      new CASTFunctionOp(new CASTIdentifier("defined"), defName)), subtree)
  );    
  
  return result;
}

CASTStatement *CBEInterface::buildClientHeader()

{
  CASTStatement *subtree = NULL;

  if (!aoi->attributes->isEmpty())
    warning("Not implemented: Attributes");
    
  addTo(subtree, buildTitleComment());
  addTo(subtree, buildReferenceDefinition());
  forAll(aoi->constants, addTo(subtree, getConstant(item)->buildDefinition()));
  forAll(aoi->types, addTo(subtree, getType(item)->buildDefinition()));
  forAll(aoi->exceptions, addTo(subtree, getType(item)->buildDefinition()));
  forAll(aoi->operations, addTo(subtree, getOperation(item)->buildClientHeader()));
  forAllBE(inheritedOps, addTo(subtree, ((CBEOperation*)item)->buildClientHeader()));
  forAll(aoi->interfaces, addTo(subtree, getInterface(item)->buildClientHeader()));
  
  return subtree;
}

CASTStatement *CBEInterface::buildServerHeader()

{
  CASTIdentifier *prefix = buildIdentifier();
  CASTStatement *subtree = NULL;

  prefix->capitalize();
  addTo(subtree, buildTitleComment());
  addTo(subtree, buildServerDefinitions());
  addTo(subtree, service->buildServerDeclarations(prefix, getVtableSize(), getItableSize(), getKtableSize()));
  addTo(subtree, new CASTSpacer());
  forAll(aoi->constants, addTo(subtree, getConstant(item)->buildDefinition()));
  forAll(aoi->types, addTo(subtree, getType(item)->buildDefinition()));
  forAll(aoi->exceptions, addTo(subtree, getType(item)->buildDefinition()));
  forAll(aoi->operations, addTo(subtree, getOperation(item)->buildServerHeader()));
  forAllBE(inheritedOps, addTo(subtree, ((CBEOperation*)item)->buildServerHeader()));
  forAll(aoi->interfaces, addTo(subtree, getInterface(item)->buildServerHeader()));
  
  return subtree;
}

CASTStatement *CBEInterface::buildVtableDeclaration()

{
  CASTStatement *result = NULL;
  CASTExpression *itable = NULL;
  
  CASTIdentifier *vtableSizeMacro = buildIdentifier();
  vtableSizeMacro->capitalize();
  vtableSizeMacro->addPostfix("_DEFAULT_VTABLE_SIZE");

  CASTIdentifier *discardVtableName = buildIdentifier();
  discardVtableName->addPostfix("_vtable_discard");

  int iidMin, iidMax, fidMin, fidMax, kidMin, kidMax;
  getIDRange(&iidMin, &iidMax, &fidMin, &fidMax, &kidMin, &kidMax);

  for (int iid=0;iid<iidMin;iid++)
    addTo(itable, discardVtableName->clone());

  for (int iid=iidMin;iid<=iidMax;iid++)
    {
      bool foundFuncWithThisIID = false;
      CAoiScope *definitionScope = NULL;
      
      for (int fid=fidMin;fid<=fidMax;fid++)
        if (getFunctionForID(fid, iid, &definitionScope))
          foundFuncWithThisIID = true;
          
      if (foundFuncWithThisIID)
        {
          CASTIdentifier *vtableName = buildIdentifier();
          vtableName->addPostfix("_vtable");
          if (iidMin!=iidMax)
            {
              char *postfix = mprintf("_%d", iid);
              vtableName->addPostfix(postfix);
              mfree(postfix);
            }  

          addTo(result, new CASTDeclarationStatement(
            voidType->buildDeclaration(
              new CASTDeclarator(
                vtableName,
                new CASTPointer(1),
                vtableSizeMacro->clone(),
                (iidMin==iidMax) ? buildDefaultVtableName() : buildVtableName(iid))))
          );
          
          addTo(itable, vtableName->clone());
        } else addTo(itable, discardVtableName->clone());
    }   

  int itableSize = getItableSize();
  for (int iid=iidMax+1;iid<itableSize;iid++)
    addTo(itable, discardVtableName->clone());

  if (iidMin!=iidMax)
    addTo(result, new CASTDeclarationStatement(
      voidType->buildDeclaration(
        new CASTDeclarator(
          discardVtableName,
          new CASTPointer(1),
          vtableSizeMacro->clone(),
          buildVtableName(UNKNOWN))))
      );

  CASTIdentifier *itableName = buildIdentifier();
  itableName->addPostfix("_itable");
  
  if (iidMin!=iidMax)
    addTo(result, new CASTDeclarationStatement(
      voidType->buildDeclaration(
        new CASTDeclarator(
          itableName,
          new CASTPointer(2),
          new CASTIntegerConstant(itableSize),
          new CASTArrayInitializer(itable))))
      );

  if (containsKernelMessages())
    {
      CASTIdentifier *ktableName = buildIdentifier();
      ktableName->addPostfix("_ktable");
      
      CASTIdentifier *ktableSizeMacro = buildIdentifier();
      ktableSizeMacro->capitalize();
      ktableSizeMacro->addPostfix("_DEFAULT_KTABLE_SIZE");

      CASTIdentifier *defaultKtable = buildIdentifier();
      defaultKtable->capitalize();
      defaultKtable->addPostfix("_DEFAULT_KTABLE");
      
      addTo(result, new CASTDeclarationStatement(
        voidType->buildDeclaration(
          new CASTDeclarator(
            ktableName,
            new CASTPointer(1),
            ktableSizeMacro,
            defaultKtable)))
      );
    }

  addTo(result, new CASTSpacer());
  
  return result;
}  

CASTStatement *CBEInterface::buildServerTemplate()

{
  CASTStatement *subtree = NULL;

  if ((globals.flags & FLAG_MODULESONLY) && ((aoi->parent)->parent == 0))
    return NULL;

  int iidMin, iidMax, fidMin, fidMax, kidMin, kidMax;
  getIDRange(&iidMin, &iidMax, &fidMin, &fidMax, &kidMin, &kidMax);

  if (!(globals.flags & FLAG_LOOPONLY))
    {
      addTo(subtree, buildTitleComment());
      forAll(aoi->operations, 
        {
          addTo(subtree, getOperation(item)->buildServerTemplate());
          if (debug_mode&DEBUG_TESTSUITE)
            addTo(subtree, getOperation(item)->buildTestFunction());
        }
      );    
      forAllBE(inheritedOps, 
        addTo(subtree, ((CBEInheritedOperation*)item)->buildServerTemplate())
      );
      forAll(aoi->interfaces, 
        addTo(subtree, getInterface(item)->buildServerTemplate())
      );
    }

  addTo(subtree, buildVtableDeclaration());

  /* As a special optimization, we do not use uTables when only kernel
     messages need to be dispatched */

  CASTIdentifier *utableRef = NULL;
  if (containsUserMessages() || !containsKernelMessages())
    {
      utableRef = buildIdentifier();
      utableRef->addPostfix((iidMin==iidMax) ? "_vtable" : "_itable");
    }  

  CASTIdentifier *ktableRef = NULL;
  if (containsKernelMessages())
    {
      ktableRef = buildIdentifier();
      ktableRef->addPostfix("_ktable");
    }  

  CASTIdentifier *prefix = buildIdentifier();
  prefix->capitalize();  
  addTo(subtree, new CASTDeclarationStatement(
    new CASTDeclaration(
      new CASTTypeSpecifier(new CASTIdentifier("void")),
      new CASTDeclarator(
        buildServerFuncName(),
        new CASTEmptyDeclaration()),
      new CASTCompoundStatement(service->buildServerLoop(prefix, utableRef, ktableRef, (iidMin!=iidMax), containsKernelMessages()))))
  );
  addTo(subtree, new CASTSpacer());
  
  /***************************************************************************/

  if (!(globals.flags & FLAG_LOOPONLY))  
    {
      CASTIdentifier *discardFuncName = buildIdentifier();
      discardFuncName->addPostfix("_discard");

      CASTStatement *discardFunction = NULL;
      if (debug_mode&DEBUG_TESTSUITE)
        addTo(discardFunction, new CASTExpressionStatement(
          new CASTFunctionOp(
            new CASTIdentifier("enter_kdebug"),
            new CASTStringConstant(false, mprintf("Discarding request for interface %s", aoi->scopedName))))
        );
      addWithTrailingSpacerTo(subtree, new CASTDeclarationStatement(
        new CASTDeclaration(
          new CASTTypeSpecifier(mprintf("void")),
          new CASTDeclarator(
            discardFuncName,
            new CASTEmptyDeclaration()),
          new CASTCompoundStatement(discardFunction)))
      );
    }
  
  return subtree;
}

bool CBEInterface::containsKernelMessages()

{
  forAll(aoi->operations, 
    if (((CAoiOperation*)item)->flags & FLAG_KERNELMSG)
      return true;
  );
  
  forAll(aoi->superclasses,
    if (getInterface(((CAoiRef*)item)->ref)->containsKernelMessages())
      return true;
  );
  
  return false;
}

bool CBEInterface::containsUserMessages()

{
  forAll(aoi->operations, 
    if (!(((CAoiOperation*)item)->flags & FLAG_KERNELMSG))
      return true;
  );
  
  forAll(aoi->superclasses,
    if (getInterface(((CAoiRef*)item)->ref)->containsUserMessages())
      return true;
  );
  
  return false;
}

void CBEInterface::getIDRange(int *iidMin, int *iidMax, int *fidMin, int *fidMax, int *kidMin, int *kidMax)

{
  *iidMin = *iidMax = aoi->iid;
  *fidMin = *kidMin = 1<<30;
  *fidMax = *kidMax = 0;

  forAll(aoi->operations, 
    { 
      int opID = getOperation(item)->getID();
      if (((CAoiOperation*)item)->flags & FLAG_KERNELMSG)
        {
          *kidMin = (opID<*kidMin) ? opID : *kidMin;
          *kidMax = (opID>*kidMax) ? opID : *kidMax;
        } else {
                 *fidMin = (opID<*fidMin) ? opID : *fidMin;
                 *fidMax = (opID>*fidMax) ? opID : *fidMax;
               }
    });
    
  forAll(aoi->superclasses, 
    { 
      int sciidMin;
      int sciidMax;
      int scfidMin;
      int scfidMax;
      int sckidMin;
      int sckidMax;
      getInterface(((CAoiRef*)item)->ref)->getIDRange(&sciidMin, &sciidMax, &scfidMin, &scfidMax, &sckidMin, &sckidMax);
      *fidMin = (scfidMin<*fidMin) ? scfidMin : *fidMin;
      *fidMax = (scfidMax>*fidMax) ? scfidMax : *fidMax;
      *iidMin = (sciidMin<*iidMin) ? sciidMin : *iidMin;
      *iidMax = (sciidMax>*iidMax) ? sciidMax : *iidMax; 
      *kidMin = (sckidMin<*kidMin) ? sckidMin : *kidMin;
      *kidMax = (sckidMax>*kidMax) ? sckidMax : *kidMax;
    });
   
  if (*fidMin == (1<<30))
    *fidMin = *fidMax;

  if (*kidMin == (1<<30))
    *kidMin = *kidMax;
}

CASTIdentifier *CBEInterface::buildServerFuncName()

{
  CASTIdentifier *serverFuncName = buildIdentifier();
  serverFuncName->addPostfix("_server");
  
  return serverFuncName;
}  

CASTIdentifier *CBEInterface::buildDefaultVtableName()

{
  CASTIdentifier *vtableName = buildIdentifier();
  vtableName->capitalize();
  vtableName->addPostfix("_DEFAULT_VTABLE");
  return vtableName;
}

CASTIdentifier *CBEInterface::buildKtableName()

{
  CASTIdentifier *ktableName = buildIdentifier();
  ktableName->capitalize();
  ktableName->addPostfix("_DEFAULT_KTABLE");
  return ktableName;
}

CASTIdentifier *CBEInterface::buildVtableName(int iid)

{
  CASTIdentifier *vtableName = buildDefaultVtableName();
  if (iid!=UNKNOWN)
    {
      char *postfix = mprintf("_%d", iid);
      vtableName->addPostfix(postfix);
      mfree(postfix);
    } else vtableName->addPostfix("_DISCARD");
  return vtableName;
}

int CBEInterface::getVtableSize()

{
  /* The vtable needs to be big enough to cover all functions, including
     those inherited from a superclass. The size should also be a power of 2,
     because we can then do the boundary check with a simple AND */

  int iidMin, iidMax, fidMin, fidMax, kidMin, kidMax;
  getIDRange(&iidMin, &iidMax, &fidMin, &fidMax, &kidMin, &kidMax);
  
  int numVtableElements = 1;
  while (numVtableElements<=fidMax)
    numVtableElements *= 2;
    
  return numVtableElements;
}  

int CBEInterface::getKtableSize()

{
  int iidMin, iidMax, fidMin, fidMax, kidMin, kidMax;
  getIDRange(&iidMin, &iidMax, &fidMin, &fidMax, &kidMin, &kidMax);
  
  if (!containsKernelMessages())
    return 0;
  
  int numKtableElements = 1;
  while (numKtableElements<=kidMax)
    numKtableElements *= 2;
    
  return numKtableElements;
}  

int CBEInterface::getItableSize()

{
  int iidMin, iidMax, fidMin, fidMax, kidMin, kidMax;
  getIDRange(&iidMin, &iidMax, &fidMin, &fidMax, &kidMin, &kidMax);
  
  if (iidMin == iidMax) 
    return 0;
  
  int numItableElements = 1;
  while (numItableElements<=iidMax)
    numItableElements *= 2;
    
  return numItableElements;
}  

CASTStatement *CBEInterface::buildServerDefinitions()

{
  CAoiScope *definitionScope = NULL;
  CASTStatement *subtree = NULL;

  CASTDeclarator *serverLoopDeclarator = new CASTDeclarator(
    buildServerFuncName(),
    new CASTEmptyDeclaration()
  );

  /* We should not assume that server loops can _never_ return */
  if (debug_mode & DEBUG_TESTSUITE)
    serverLoopDeclarator->setAttributes(
      new CASTAttributeSpecifier(
        new CASTIdentifier("noreturn"))
    );

  addTo(subtree, new CASTDeclarationStatement(
    getType(aoiRoot->lookupSymbol("#void", SYM_TYPE))->buildDeclaration(
      serverLoopDeclarator))
  );
    
  CASTIdentifier *discardFuncName = buildIdentifier();
  discardFuncName->addPostfix("_discard");

  addTo(subtree, 
    new CASTDeclarationStatement(
      getType(aoiRoot->lookupSymbol("#void", SYM_TYPE))->buildDeclaration(
        new CASTDeclarator(
          discardFuncName,
          new CASTEmptyDeclaration()
        )))
  );        
  
  addTo(subtree, new CASTSpacer());  

  /* Collect the function pointers. For each ID, we ask the superclasses 
     for a pointer; it nothing is returned, we insert the discard function */
  
  int numVtableElements, iidMin, iidMax, fidMin, fidMax, kidMin, kidMax;
  numVtableElements = getVtableSize();
  getIDRange(&iidMin, &iidMax, &fidMin, &fidMax, &kidMin, &kidMax);

  for (int iid=iidMin;iid<=iidMax;iid++)
    {
      bool foundFuncWithThisIID = false;
      definitionScope = NULL;
      
      for (int fid=fidMin;fid<=fidMax;fid++)
        if (getFunctionForID(fid, iid, &definitionScope))
          foundFuncWithThisIID = true;

      if (foundFuncWithThisIID || (iidMin==iidMax))
        {
          CASTExpression *functionPointers = NULL;
          for (int fid=0;fid<numVtableElements;fid++)
            {
              CAoiOperation *thisFunc;
              CASTExpression *newElement;
              
              definitionScope = NULL;
              thisFunc = getFunctionForID(fid, iid, &definitionScope);
              if (thisFunc != NULL)
                {
                  CASTIdentifier *funcName = buildIdentifier();
                  funcName->addPrefix("service_");
                  funcName->addPostfix("_");
                  funcName->addPostfix(thisFunc->name);
                  newElement = funcName;
                } else newElement = discardFuncName->clone();
                
              if (globals.mapping == MAPPING_CXX)
                newElement = new CASTUnaryOp("(void*)&", newElement);
              
              addTo(functionPointers, newElement);
            }
            
         addTo(subtree, new CASTPreprocDefine(
           (iidMin==iidMax) ? buildDefaultVtableName() : buildVtableName(iid), 
           new CASTArrayInitializer(functionPointers))
         );  
        }
    }
    
  if (iidMin!=iidMax)
    {
      CASTExpression *functionPointers = NULL;
      for (int fid=0;fid<numVtableElements;fid++)
        addTo(functionPointers, new CASTUnaryOp("(void*)&", discardFuncName->clone()));
        
      addTo(subtree, new CASTPreprocDefine(
        buildVtableName(UNKNOWN),
        new CASTArrayInitializer(functionPointers))
      );
    }
    
  CASTIdentifier *vtableSizeMacro = buildIdentifier();
  vtableSizeMacro->capitalize();
  vtableSizeMacro->addPostfix("_DEFAULT_VTABLE_SIZE");
  addTo(subtree, new CASTPreprocDefine(
    vtableSizeMacro,
    new CASTIntegerConstant(getVtableSize()))
  );

  CASTIdentifier *maxFidMacro = buildIdentifier();
  maxFidMacro->capitalize();
  maxFidMacro->addPostfix("_MAX_FID");
  addTo(subtree, new CASTPreprocDefine(
    maxFidMacro,
    new CASTIntegerConstant(fidMax))
  );

  if ((kidMin!=kidMax) || getFunctionForID(kidMin, IID_KERNEL, &definitionScope))
    {
      CASTExpression *functionPointers = NULL;
      int maxKid = getKtableSize();
      
      for (int kid=0;kid<maxKid;kid++)
        {
          CASTExpression *newElement;
          CAoiOperation *thisFunc;
          thisFunc = getFunctionForID(kid, IID_KERNEL, &definitionScope);
          if (thisFunc != NULL)
            {
              CASTIdentifier *funcName = buildIdentifier();
              funcName->addPrefix("service_");
              funcName->addPostfix("_");
              funcName->addPostfix(thisFunc->name);
              newElement = funcName;
            } else newElement = discardFuncName->clone();
            
          if (globals.mapping == MAPPING_CXX)
            newElement = new CASTUnaryOp("(void*)&", newElement);
            
          addTo(functionPointers, newElement);
        }
            
      addTo(subtree, new CASTPreprocDefine(
        buildKtableName(), 
        new CASTArrayInitializer(functionPointers))
      );  

      CASTIdentifier *ktableSizeMacro = buildIdentifier();
      ktableSizeMacro->capitalize();
      ktableSizeMacro->addPostfix("_DEFAULT_KTABLE_SIZE");
      addTo(subtree, new CASTPreprocDefine(
        ktableSizeMacro,
        new CASTIntegerConstant(getKtableSize()))
      );
    }

  return subtree;      
}

CAoiOperation *CBEInterface::getFunctionForID(int fid, int iid, CAoiScope **definitionScope)

{
  CAoiOperation *result = NULL;

  forAll(aoi->operations, 
    if ((fid==getOperation(item)->getID()) &&
        (((iid == aoi->iid) && (!(((CAoiOperation*)item)->flags & FLAG_KERNELMSG))) || 
         ((iid == IID_KERNEL) && (((CAoiOperation*)item)->flags & FLAG_KERNELMSG))))
      {
        if (result)
          {
            semanticError(result->context, "function ID collision");
            semanticError(item->context, "in this scope");
          }
              
        result = (CAoiOperation*)item;
        *definitionScope = (CAoiScope*)item->parent;
      }
  );

  forAll(aoi->superclasses, 
    { 
      CAoiScope *previousScope = NULL;
      CAoiOperation *scResult = getInterface(((CAoiRef*)item)->ref)->getFunctionForID(fid, iid, &previousScope);

      if (scResult)
        {
          if (!result || result==scResult)
            {
              result = scResult;
              *definitionScope = previousScope;
            } else semanticError(result->context, "function ID %d conflicts with %s::%s (suggest different UUID for %s)", 
                                 fid, previousScope->name, scResult->name, (*definitionScope)->name);
        }
    }
  );
    
  return result;
}

