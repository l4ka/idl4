#include "fe/c++.h"
#include "cast.h"
#include "string.h"

int debug_mode = 0;

/* A visitor that replaces one type with another. Here it is used to change all
   occurrences of the 'Generic' type to 'Specific' */

class ReplaceTypeVisitor : public CASTDFSVisitor

{
public:
  const char *from, *to;
  ReplaceTypeVisitor(const char *from, const char *to) : CASTDFSVisitor()
    { this->from = from; this->to = to; }
  virtual void visit(CASTTypeSpecifier *peer)
    {
      if (!strcmp(peer->identifier->identifier, from))
        peer->identifier = new CASTIdentifier(to);
    }
};

/* A visitor that appends statements to a specific class. Here it is used to
   move the non-virtual methods from 'Generic' to 'Specific' */

class InsertVisitor : public CASTDFSVisitor

{
public:
  const char *target;
  CASTStatement *what;
  InsertVisitor(const char *target, CASTStatement *what) : CASTDFSVisitor()
    { this->target = target; this->what = what; }
  virtual void visit(CASTAggregateSpecifier *peer)
    { if (!strcmp(peer->identifier->identifier, target)) peer->declarations->add(what); }
};  

/* A visitor that strips the 'virtual' specifier from all declarations. This is
   necessary because the methods in 'Specific' are still virtual */

class DevirtualizeMethodsVisitor : public CASTDFSVisitor

{
public:
  bool killMe;
  const char *target;
  DevirtualizeMethodsVisitor(const char *target) : CASTDFSVisitor()
    { this->target = target; killMe = false; }
  virtual void visit(CASTStorageClassSpecifier *peer)
    { if (!strcmp(peer->keyword, "virtual")) killMe = true; }
  virtual void visit(CASTDeclaration *peer)
    { 
      CASTDeclarationSpecifier *iterator = peer->specifiers;
      if (iterator)
        do {
             iterator->accept(this);
             
             // special magic must be applied when we are about to kill
             // the first specifier of this declaration
             
             if (killMe)
               {
                 if (iterator==peer->specifiers)
                   {
                     if (iterator->next == iterator)
                       { peer->specifiers = NULL; break; }
                       else { peer->specifiers = (CASTDeclarationSpecifier*)iterator->next; iterator->remove(); }
                   } else iterator->remove();
                 killMe = false; iterator = peer->specifiers;  
               } else iterator = (CASTDeclarationSpecifier*)iterator->next;
           } while (iterator!=peer->specifiers);
    }
};  

/* A visitor that removes all virtual methods. This is necessary because the
   virtual methods in the 'Generic' class are all overloaded by their counterparts
   in 'Specific' (of course, this might not always hold) */

class KillVirtualMethodsVisitor : public CASTDFSVisitor

{
public:
  bool killMe;
  
  KillVirtualMethodsVisitor() : CASTDFSVisitor() 
    { killMe = false; }
  virtual void visit(CASTStorageClassSpecifier *peer)
    { if (!strcmp(peer->keyword, "virtual")) killMe = true; }
  virtual void visit(CASTDeclarationStatement *peer)
    {
      CASTDFSVisitor::visit(peer);
      if (killMe)
        peer->declaration = NULL;
      killMe = false;
    } 
};

/* A visitor that kills an entire class by removing its declaration; this is
   what eventually happens to the 'Generic' class. The declarations inside
   the removed class are made available as a result */

class KillClassVisitor : public CASTDFSVisitor

{
public:
  const char *name;
  bool killMe;
  CASTStatement *result;
  
  KillClassVisitor(const char *name) : CASTDFSVisitor() 
    { killMe = false; result = NULL; this->name = name; }
  virtual void visit(CASTAggregateSpecifier *peer)
    { 
      if (!strcmp(peer->identifier->identifier, name)) 
        { result = peer->declarations; killMe=true; } 
        
      // Other classes might have specified the candidate class as their
      // base class. This is no longer necessary; we remove the specifier
        
      CASTBaseClass *iterator = peer->parents;
      if (iterator)
        do {
             if (!strcmp(iterator->identifier->identifier, name))
               {
                 if (iterator->next == iterator)
                   { peer->parents = NULL; break; }
                   else iterator->remove();
               }
             iterator = (CASTBaseClass*)iterator->next;
           } while (iterator!=peer->parents);
    }
  virtual void visit(CASTDeclarationStatement *peer)
    {
      CASTDFSVisitor::visit(peer);
      if (killMe)
        peer->declaration = NULL;
      killMe = false;
    }
};

int main(int argc, char *argv[])

{
  if (argc!=3)
    panic("Usage: weaver <input.c> <output.c>");

  // Parse the C++ input file and deliver the Abstract Syntax Tree (AST)
  
  CPPInputContext *inFile = new CPPInputContext(argv[1]);
  CASTBase *tree = inFile->Parse();
  
  if (inFile->hasErrors())
    panic("%s: %i Error(s) and %i Warning(s)", argv[1],
          inFile->getErrorCount(), inFile->getWarningCount());
          
  // Apply the transformations
          
  KillClassVisitor *genericKiller = new KillClassVisitor("Generic");
  tree->acceptAll(genericKiller);
  genericKiller->result->acceptAll(new KillVirtualMethodsVisitor());
  tree->acceptAll(new ReplaceTypeVisitor("Generic", "Specific"));
  tree->acceptAll(new InsertVisitor("Specific", genericKiller->result));
  tree->acceptAll(new DevirtualizeMethodsVisitor("Specific"));
          
  // Write the resulting tree back to a C++ file
          
  (new CASTFile(argv[2], tree))->write();
         
  return 0;
}
