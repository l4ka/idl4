#include <string.h>

#include "cast.h"
#include <parser.hh>

#define dprint(a...) do { if (debug_mode&DEBUG_IMPORT) fprintf(stderr, a); } while (0)

class TCFSymbol

{
public:
  TCFSymbol *next, *parentScope;
  bool isTransparent;
  int precond;
  int type;
  const char *name;

  TCFSymbol(int type, const char *name, int precond, bool isTransparent, TCFSymbol *parentScope);
  TCFSymbol *lookupSymbolInScope(const char *name, TCFSymbol *scope);
  int lookupType(const char *name);
  void add(TCFSymbol *sym);
  TCFSymbol *get(const char *name);
};

TCFSymbol::TCFSymbol(int type, const char *name, int precond, bool isTransparent, TCFSymbol *parentScope)

{
  this->next = NULL;
  this->name = name;
  this->type = type;
  this->precond = precond;
  this->parentScope = parentScope;
  this->isTransparent = isTransparent;
}
  
void TCFSymbol::add(TCFSymbol *sym)

{
  sym->next = next;
  next = sym;
}

static TCFSymbol *symtab = NULL;
static TCFSymbol *currentScope = NULL, *parseScope = NULL;
static int currentPrecond = 0;
static bool globalAllowed = true;
static int nestLevel = 0;
bool cppParseMode = true;

void initSymtab()

{
  symtab = new TCFSymbol(IDENTIFIER, NULL, 0, false, NULL);
  symtab->add(new TCFSymbol(TYPENAME, "__builtin_va_list", 0, false, NULL));
  symtab->add(new TCFSymbol(TYPENAME, "__builtin_ptrdiff_t", 0, false, NULL));
  symtab->add(new TCFSymbol(TYPENAME, "__builtin_size_t", 0, false, NULL));
  currentScope = NULL;
}

TCFSymbol *TCFSymbol::lookupSymbolInScope(const char *name, TCFSymbol *scope)

{
  TCFSymbol *iterator = next;

// fprintf(stderr, "looking for %s in scope %s (precond=%d)\n", name, (scope==NULL) ? "root" : scope->name, currentPrecond);
 
  while (iterator)
    {
      if (((iterator->parentScope == scope) || (iterator == scope)) &&
          !strcmp(name, iterator->name) && 
          (currentPrecond==iterator->precond || cppParseMode))
        return iterator;
      
      if (iterator->parentScope == scope && iterator->isTransparent)
        {
          TCFSymbol *result = lookupSymbolInScope(name, iterator);
          if (result)
            return result;
        }
        
      iterator = iterator->next;
    }  
      
  return NULL;
}

int TCFSymbol::lookupType(const char *name)

{
  TCFSymbol *scope = parseScope;
  
  do {
       TCFSymbol *result = lookupSymbolInScope(name, scope);
       if (result)
         {
           if (result == scope)
             return SELFNAME;
             else return result->type;
         }
           
       /* try parent scope */
           
       if (scope!=NULL)
         scope = scope->parentScope;
         else break;
           
     } while (1);
      
  return IDENTIFIER;
}

int what_is(const char *sym)

{
  dprint("Symtab: %s -> ", sym);

  switch (symtab->lookupType(sym))
    {
      case TYPENAME     : dprint("TYPENAME\n");return TYPENAME;
      case NSNAME       : dprint("NSNAME\n");return NSNAME;
      case SELFNAME	: dprint("SELFNAME\n");return SELFNAME;
      default           : dprint("IDENTIFIER\n");return IDENTIFIER;
    }  
}

void addSymbol(int type, const char *sym)

{
  dprint("Symtab: added %s (", sym);
  
  switch (type)
    {
      case TYPENAME : dprint("TYPENAME)"); break;
      case NSNAME   : dprint("NSNAME)"); break;
      default       : panic("Weird symbol type"); break;
    }
    
  dprint(" to current scope %s\n", (currentScope==NULL) ? "root" : currentScope->name);

  if (symtab->lookupSymbolInScope(sym, currentScope))
    panic("defined twice: %s", sym);
    
  symtab->add(new TCFSymbol(type, sym, 0, false, currentScope));
}

void enterNewScope(int type, const char *sym, int precond, bool isTransparent)

{
  dprint("Symtab: added symbol as new scope: %s (", sym);
  
  switch (type)
    {
      case TYPENAME : dprint("TYPENAME"); break;
      case NSNAME   : dprint("NSNAME"); break;
      default       : panic("Weird symbol type\n"); break;
    }
    
  switch (precond)
    {
      case STRUCT   : dprint(", precond=STRUCT"); break;
      case UNION    : dprint(", precond=UNION"); break;
      case NAMESPACE: dprint(", precond=NAMESPACE"); break;
      case 0        : dprint(", precond=none"); break;
      default       : panic("Weird precondition\n"); break;
    }
    
  dprint(")\n");
  
  TCFSymbol *myself = symtab->lookupSymbolInScope(sym, currentScope);
  if (!(myself!=NULL && myself->type == type && myself->precond == precond))
    {
      myself = new TCFSymbol(type, sym, precond, isTransparent, currentScope);
      symtab->add(myself);
    } else dprint("Already exists; only moving scope\n");  
    
  if (myself == currentScope)
    nestLevel ++;
    
  currentScope = myself;
  parseScope = myself;
}

void leaveScope()

{
  if (nestLevel>0)
    {
      nestLevel--;
      dprint("Leaving nested scope\n");
      return;
    }

  if (currentScope==NULL)
    panic("Trying to leave root scope");
    
  dprint("Symtab: scope ends: %s\n", currentScope->name);
  currentScope = currentScope->parentScope;
}

void setPrecond(int newPrecond)

{
  dprint("Symtab: set precondition: ");
  switch (newPrecond)
    {
      case STRUCT    : dprint("STRUCT\n"); break;
      case UNION     : dprint("UNION\n"); break;
      case NAMESPACE : dprint("NAMESPACE\n"); break;
      case 0         : dprint("none\n"); break;
      default        : panic("Unknown precondition: %d", newPrecond);
    }
    
  currentPrecond = newPrecond;
}

void selectScope(const char *scopeName)

{
  dprint("# Selected subscope: %s (global: %d)\n", scopeName, (int )globalAllowed);
  
  TCFSymbol *scope = parseScope;
  
  do {
       TCFSymbol *result = symtab->lookupSymbolInScope(scopeName, scope);
       if (result && result->type==TYPENAME)
         {
           globalAllowed = false;
           parseScope = result;
           return;
         }
           
       /* try parent scope */
           
       if (scope!=NULL)
         scope = scope->parentScope;
         else break;
           
     } while (1);
  
  panic("Cannot open subscope: %s (not found in %s%s)", scopeName,
        (parseScope==NULL) ? "root" : parseScope->name,
        globalAllowed ? " or global scope" : ""
        );
}

void selectCurrentScope()

{
  parseScope = currentScope;
  dprint("# Selected current scope (%s)\n", (currentScope==NULL) ? "root" : currentScope->name);
  globalAllowed = true;
}
