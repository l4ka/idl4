#include <string.h>
#include <sys/stat.h>

#include "aoi.h"
#include "cast.h"
#include "fe/c++.h"
#include "fe/idl.h"

#include "dump.h"

#define currentScope  (currentIDLContext->getCurrentScope())
#define lookupType(a) ((CAoiType*)currentScope->lookupSymbol(a, SYM_TYPE))

static const char *inFile = "(unknown)";
static CASTBase *tree = NULL;

// ==========================================================================

class CASTImportVisitor : public CASTDFSVisitor

{
public:
  static CAoiList *result;
  bool madeProgress;
  
  CASTImportVisitor() : CASTDFSVisitor()
    { result = aoiFactory->buildList(); madeProgress = false; };
  virtual void visit(CASTDeclarationStatement *peer);
  virtual void visit(CASTAggregateSpecifier *peer);
};

class CASTTypedefFinder : public CASTDFSVisitor

{
public:
  bool foundTypedef;
  
  CASTTypedefFinder() : CASTDFSVisitor()
    { foundTypedef = false; };
  virtual void visit(CASTStorageClassSpecifier *peer)
    { if (!strcmp(peer->keyword, "typedef")) foundTypedef = true; };
};

class CASTMemberExtractor : public CASTDFSVisitor

{
  CAoiList *declarators;

public:
  CAoiList *result;
  
  CASTMemberExtractor() : CASTDFSVisitor()
    { result = aoiFactory->buildList(); declarators = NULL; };
  virtual void visit(CASTDeclarator *peer);
  virtual void visit(CASTDeclarationStatement *peer);
};

class CASTTypeExtractor : public CASTDFSVisitor

{
  CAoiType *result;
  bool isSigned;
  bool isLong;
  int size;
  
public:
  bool failed;

  CASTTypeExtractor() : CASTDFSVisitor()
    { isSigned = true; isLong = false; failed = false; size = 8*globals.word_size; result = NULL; };
  virtual void visit(CASTTypeSpecifier *peer);
  virtual void visit(CASTAggregateSpecifier *peer);
  CAoiType *getType();
};

class CASTValueExtractor : public CASTDFSVisitor

{
public:
  int result;
  
  CASTValueExtractor() : CASTDFSVisitor()
    { result = 0; };
  virtual void visit(CASTIntegerConstant *peer)
    { this->result = peer->value; };
};

CAoiList *CASTImportVisitor::result = NULL;

CAoiContext *getDefaultContext()

{
  return new CAoiContext(inFile, "", 0, 0);
}

CAoiType *lookupImported(const char *typeName)

{
  assert(CASTImportVisitor::result);
  
  CAoiType *iterator = (CAoiType*)CASTImportVisitor::result->getFirstElement();
  while (!iterator->isEndOfList())
    {
      if (!strcmp(iterator->name, typeName))
        return iterator;
        
      iterator = (CAoiType*)iterator->next;
    }
    
  return NULL;
}

// ==========================================================================
// CASTImportVisitor: Walks the file, finds all the type definitions and 
//                    tries to import them using a CASTTypeExtractor

void CASTImportVisitor::visit(CASTAggregateSpecifier *peer)

{
  CASTDFSVisitor::visit(peer);

  /* We do not parse forward declarations here because the actual type
   * is imported later. We also do not parse anonymous structs and unions
   * because they cannot be referred to from the IDL file (they _will_
   * be considered by the TypeExtractor, though) 
  */

  if (!peer->identifier || !peer->declarations)
    return;

  /* If the type has already been imported in a previous iteration,
     we return immediately. */

  if (lookupImported(peer->identifier->identifier) != NULL)
    return;

  if (debug_mode & DEBUG_IMPORT)
    {
      peer->write();
      printf("\n");
    }

  /* Attempt to convert the type from CAST to AOI. If this fails,
     e.g. because a member type has been declared forward and is
     still missing, we defer this step. */

  CASTTypeExtractor *extractor = new CASTTypeExtractor();
  peer->accept(extractor);
  if (extractor->failed)
    return;

  CAoiType *newType = extractor->getType();
  assert(newType);
  
  if (debug_mode & DEBUG_IMPORT)
    {
      printf(">>> IMPORTED STRUCT\n");
      newType->accept(new CAoiDumpVisitor());
      printf("********************************************************\n");
    }

  result->add(newType);
  madeProgress = true;
}  

void CASTImportVisitor::visit(CASTDeclarationStatement *peer)

{
  assert(peer->declaration);

  CASTDFSVisitor::visit(peer);
  
  CASTTypedefFinder *typedefFinder = new CASTTypedefFinder();
  peer->declaration->accept(typedefFinder);
  
  if (typedefFinder->foundTypedef)
    {
      assert(peer->declaration->declarators);
      assert(peer->declaration->declarators->identifier);

      if (debug_mode & DEBUG_IMPORT)
        peer->write();

      /* Maybe the type has been imported in a previous iteration.
       * If this is the case, we return immediately. */

      if (lookupImported(peer->declaration->declarators->identifier->identifier) != NULL)
        return;
    
      /* We attempt to convert the alias type from CAST to AOI. If this
         fails, e.g. because a member type is not yet known,
         we return immediately. */
    
      CASTTypeExtractor *extractor = new CASTTypeExtractor();
      peer->declaration->accept(extractor);
      if (extractor->failed)
        return;
      
      CAoiType *definedType = extractor->getType();
      CAoiType *newType = aoiFactory->buildAliasType(
        definedType, 
        aoiFactory->buildDeclarator(peer->declaration->declarators->identifier->identifier, getDefaultContext()),
        currentScope, getDefaultContext()
      );
      
      if (!(debug_mode & DEBUG_IMPORT))
        newType->flags |= FLAG_DONTDEFINE;
 
      newType->flags |= FLAG_DONTSCOPE;

      if (debug_mode & DEBUG_IMPORT)
        {
          printf(">>> IMPORTED ALIAS TYPE\n");
          newType->accept(new CAoiDumpVisitor());
          printf("********************************************************\n");
        }
      
      result->add(newType);
      madeProgress = true;
    }
}

// ==========================================================================
// CASTTypeExtractor: Converts a CAST subtree into an AOI data type

void CASTTypeExtractor::visit(CASTAggregateSpecifier *peer)

{
  CASTDFSVisitor::visit(peer);

  CASTMemberExtractor *extractor = new CASTMemberExtractor();
  if (peer->declarations)
    peer->declarations->acceptAll(extractor);
  
  if (extractor->result->isEmpty())
    {
      /* The struct appears to have no members; maybe it is a forward
         declaration. There is a chance that it has been imported in
         a previous iteration. */
    
      if (peer->identifier)
        {
          result = lookupImported(peer->identifier->identifier);
          if (result)
            return;
        }
          
      failed = true;
      return;
    }
  
  CAoiType *newType = NULL;
  if (!strcmp(peer->type, "struct"))
    {
      assert(!extractor->result->isEmpty()); // otherwise forward struct declaration
    
      newType = aoiFactory->buildStructType(
        extractor->result,
        (peer->identifier) ? peer->identifier->identifier : NULL,
        NULL, getDefaultContext()
      );
  
      CAoiParameter *iterator = (CAoiParameter*)extractor->result->getFirstElement();
      while (!iterator->isEndOfList())
        {
          iterator->parent = newType;
          iterator = (CAoiParameter*)iterator->next;
        }
    } else {
             CAoiList *unionElements = aoiFactory->buildList();
             
             while (!extractor->result->isEmpty())
               {
                 CAoiParameter *param = (CAoiParameter*)extractor->result->removeFirstElement();
                 unionElements->add(aoiFactory->buildUnionElement(
                   NULL, param, param->context));
               }    
             
             if (!unionElements->isEmpty())
               {
                 newType = aoiFactory->buildUnionType(
                   unionElements,
                   (peer->identifier) ? peer->identifier->identifier : NULL,
                   NULL, NULL, getDefaultContext()
                 );
  
                 forAll(unionElements, ((CAoiUnionElement*)item)->parent = newType);
               } else {
                        newType = lookupImported(peer->identifier->identifier);

                        if (!newType)
                          panic("Undefined union: %s", peer->identifier->identifier);
                      }
           }
    
  result = newType;
}

void CASTTypeExtractor::visit(CASTTypeSpecifier *peer)

{
  CASTDFSVisitor::visit(peer);

  assert(peer->identifier);
  assert(peer->identifier->identifier);
  
  const char *typeName = peer->identifier->identifier;
  
  if (!strcmp(typeName, "unsigned"))
    isSigned = false; else
  if (!strcmp(typeName, "signed"))
    isSigned = true; else
  if (!strcmp(typeName, "short"))
    size = 16; else
  if (!strcmp(typeName, "int"))
    size = 8*globals.word_size; else
  if (!strcmp(typeName, "char"))
    size = 8; else
  if (!strcmp(typeName, "void"))
    result = lookupType("#void"); else
  if (!strcmp(typeName, "long"))
    {
      if (!isLong) isLong = true;
              else size = 64;
    } else {
             result = lookupImported(typeName);
             
             if (!result)
               failed = true;
           }   
}

CAoiType *CASTTypeExtractor::getType()

{
  if (!result)
    {
      char presumedName[8];
      sprintf(presumedName, "#%c%d", isSigned ? 's' : 'u', size);
      result = lookupType(presumedName);
      assert(result);
    } else result->parentScope = currentScope;

  if (!(debug_mode & DEBUG_IMPORT))
    result->flags |= FLAG_DONTDEFINE;

  result->flags |= FLAG_DONTSCOPE;
    
  return result;  
}

// ==========================================================================
// CASTMemberExtractor: Collects all declarators (e.g. members of a struct)

void CASTMemberExtractor::visit(CASTDeclarator *peer)

{
  CASTDFSVisitor::visit(peer);

  if (!declarators) 
    return;

  if (!peer->identifier)
    return;
  
  CAoiDeclarator *newDeclarator = aoiFactory->buildDeclarator(peer->identifier->identifier, getDefaultContext());
  if (peer->bitSize)
    {
      CASTValueExtractor *valueExtractor = new CASTValueExtractor();
      peer->bitSize->accept(valueExtractor);
      newDeclarator->bits = valueExtractor->result;
    }  

  if (peer->arrayDims)
    {
      CASTExpression *currentDim = peer->arrayDims;
      do {
  	   CASTValueExtractor *valueExtractor = new CASTValueExtractor();
	   currentDim->accept(valueExtractor);
	   newDeclarator->dimension[newDeclarator->numBrackets++] = valueExtractor->result;
	   if (newDeclarator->numBrackets==MAX_DIMENSION)
	     panic("Too many dimensions in array while importing types");
	   currentDim = (CASTExpression*)currentDim->next;
	 } while (currentDim!=peer->arrayDims);
    }
    
  declarators->add(newDeclarator);
}

void CASTMemberExtractor::visit(CASTDeclarationStatement *peer)

{
  /* no recursion (e.g. struct declarator inside a union) */
  if (declarators) 
    return;

  declarators = new CAoiList();

  assert(peer->declaration);
  assert(peer->declaration->declarators);

  /* collect all the declarators */
  CASTDFSVisitor::visit(peer);

  CASTTypeExtractor *extractor = new CASTTypeExtractor();
  peer->declaration->acceptAll(extractor);
  
  forAll(declarators, 
    result->add(aoiFactory->buildParameter(extractor->getType(), 
      (CAoiDeclarator*)item, aoiFactory->buildList(), getDefaultContext()))
  );
  
  declarators = NULL;
}

// ==========================================================================

CAoiList *importTypes(const char *filename, CAoiContext *context)

{
  char *thisFile;
  struct stat stat_buf;
  
  thisFile = mprintf("%s%s", globals.prefix_path, filename);
  if (stat(thisFile, &stat_buf)!=0)
    for (int i=0;globals.preproc_includes[i];i++)
      {
        mfree(thisFile);
        thisFile = mprintf("%s/%s", globals.preproc_includes[i], filename);
        if (stat(thisFile, &stat_buf)==0)
          break;
      }
  
  if (stat(thisFile, &stat_buf))
    {
      semanticError(context, "not found: %s", filename);
      return aoiFactory->buildList();
    }
  
  inFile = thisFile;
 
  CPPInputContext *inFile = new CPPInputContext(thisFile);
  inFile->setPreprocDefines(globals.preproc_defines);
  inFile->setPreprocIncludes(globals.preproc_includes);
  inFile->setPreprocOptions(globals.preproc_options);
  if (globals.cpp_path)
    inFile->setCppPath(globals.cpp_path);
  tree = inFile->Parse();

  if ((inFile->hasErrors()) || (!tree))
    {
      context->showMessage("parse errors in header file, skipping");
      return NULL;
    }  
 
  CASTImportVisitor *visitor = new CASTImportVisitor();
  do { 
        visitor->madeProgress = false;
        tree->acceptAll(visitor); 
     } while (visitor->madeProgress);
    
  return visitor->result;
}
