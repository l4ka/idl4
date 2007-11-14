#include "be.h"

CASTIdentifier *CBEVarSource::requestVariable(bool needPersistence, CBEType *requestedType, int indirLevel)

{
  assert(requestedType);

  if (this->type)
    if (this->type->equals(requestedType))
      if (indirLevel == this->indirLevel)
        if ((!needPersistence) && (!this->isPersistent))
          return new CASTIdentifier(this->name);
  
  if (this->next)
    return next->requestVariable(needPersistence, requestedType, indirLevel);

  const char *name = mprintf("%c%d", (needPersistence) ? 'g' : 'l', id+1);
  this->next = new CBEVarSource(requestedType, name, needPersistence, indirLevel, id+1);
  
  return new CASTIdentifier(name);
}


CASTStatement *CBEVarSource::declareAll()

{
  CASTStatement *result = NULL;
  CASTPointer *ptr = NULL;
  
  if (this->indirLevel)
    ptr = new CASTPointer(this->indirLevel);

  if (this->type)
    addTo(result, new CASTDeclarationStatement(
      this->type->buildDeclaration(
        new CASTDeclarator(new CASTIdentifier(name), ptr)))
    );    

  if (this->next) 
    addTo(result, next->declareAll());
    
  return result;  
}
