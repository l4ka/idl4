#include <stdio.h>
#include <string.h>

#include "aoi.h"

int semanticErrors = 0;

CAoiBase::CAoiBase(CAoiContext *context)

{
  this->parent = NULL;
  this->prev = NULL;
  this->next = NULL;
  this->name = NULL;
  this->peer = NULL;
  this->context = context;
}

void CAoiBase::add(CAoiBase *newElement)

{
  assert(newElement!=NULL);
  assert(newElement->prev==NULL);
  assert(newElement->next==NULL);
  
  newElement->next = this;
  newElement->prev = this->prev;
  (this->prev)->next = newElement;
  this->prev = newElement;
}

CAoiScope::CAoiScope(CAoiScope *parentScope, CAoiContext *context) : CAoiBase(context)

{
  this->submodules = new CAoiList();
  this->interfaces = new CAoiList();
  this->types = new CAoiList();
  this->attributes = new CAoiList();
  this->constants = new CAoiList();
  this->exceptions = new CAoiList();
  this->operations = new CAoiList();
  this->includes = new CAoiList();
  this->parent = parentScope;
  this->scopedName = NULL;
}  

CAoiBase *CAoiList::removeFirstElement()

{
  assert(!isEmpty());
  
  CAoiBase *elem = this->next;
  this->next = (this->next)->next;
  (this->next)->prev = this;
  elem->next = NULL;
  elem->prev = NULL;
  
  return elem;
}

CAoiBase *CAoiList::removeElement(CAoiBase *element)

{
  CAoiBase *iterator = getFirstElement();
  
  assert(iterator!=NULL);
  
  while (iterator!=element)
    {
      assert(!iterator->isEndOfList());
      iterator = iterator->next;
    }
    
  (iterator->next)->prev = iterator->prev;
  (iterator->prev)->next = iterator->next;
  iterator->prev = NULL;
  iterator->next = NULL;
  
  return element;
}

void CAoiList::merge(CAoiList *list)

{
  while (!list->isEmpty())
    this->add(list->removeFirstElement());
}    

bool CAoiConstInt::performOperation(AOICONSTOP oper, CAoiConstBase *argument)

{
  if (oper==OP_NEG)
    {
      value = ~value;
      return true;
    }  

  if (oper==OP_INVERT)
    {
      value = -value;
      return true;
    }  

  if (argument->isScalar())
    {
      switch (oper)
        {
          case OP_XOR    : value ^= ((CAoiConstInt*)argument)->value; break;
          case OP_OR     : value |= ((CAoiConstInt*)argument)->value; break;
          case OP_AND    : value &= ((CAoiConstInt*)argument)->value; break;
          case OP_LSHIFT : value <<= ((CAoiConstInt*)argument)->value; break;
          case OP_RSHIFT : value >>= ((CAoiConstInt*)argument)->value; break;
          case OP_ADD    : value += ((CAoiConstInt*)argument)->value; break;
          case OP_SUB    : value -= ((CAoiConstInt*)argument)->value; break;
          case OP_MUL    : value *= ((CAoiConstInt*)argument)->value; break;
          case OP_DIV    : if (((CAoiConstInt*)argument)->value)
                             value /= ((CAoiConstInt*)argument)->value; 
                             else return false;
                           break;
          case OP_MOD    : if ((((CAoiConstInt*)argument)->value)>0)
                             value %= ((CAoiConstInt*)argument)->value; 
                             else return false;
                           break;
          default        : assert(false);
        }

      delete argument;
      return true;    
    }                     
    
  return false;
}

bool CAoiConstString::performOperation(AOICONSTOP oper, CAoiConstBase *argument)

{
  return false;
}  

bool CAoiConstFloat::performOperation(AOICONSTOP oper, CAoiConstBase *argument)

{
  return false;
}  

bool CAoiConstChar::performOperation(AOICONSTOP oper, CAoiConstBase *argument)

{
  return false;
}  

CAoiBase *CAoiScope::lookupSymbol(const char *identifier, AOISYMBOLCLASS symbolType)

{
  CAoiBase *tmp;
  
  tmp = localLookup(identifier, symbolType);
  if ((!tmp) && parent)
    tmp = ((CAoiScope*)parent)->lookupSymbol(identifier, symbolType);
    
  return tmp;  
}

CAoiBase *CAoiList::getByName(const char *identifier)

{
  if (!isEmpty())
    {
      CAoiBase *iterator = getFirstElement();
      while (!(iterator->isEndOfList()))
        {
          if (iterator->name)
            if (!strcasecmp(iterator->name, identifier))
              return iterator;
         
          iterator = iterator->next;
        }
    }    
    
  return NULL;        
}

CAoiBase *CAoiScope::localLookup(const char *identifier, AOISYMBOLCLASS symbolType)

{
  if (symbolType == SYM_ANY)
    {
      CAoiBase *result = NULL;
      
      if (!result) result = localLookup(identifier, SYM_SCOPE);
      if (!result) result = localLookup(identifier, SYM_TYPE);
      if (!result) result = localLookup(identifier, SYM_CONSTANT);
      if (!result) result = localLookup(identifier, SYM_OPERATION);
      if (!result) result = localLookup(identifier, SYM_ATTRIBUTE);
      if (!result) result = localLookup(identifier, SYM_EXCEPTION);
      
      return result;
    }
    
  switch (symbolType)
    {
      case SYM_TYPE      : return types->getByName(identifier);
      case SYM_CONSTANT  : return constants->getByName(identifier);
      case SYM_OPERATION : return operations->getByName(identifier);
      case SYM_ATTRIBUTE : return attributes->getByName(identifier);
      case SYM_EXCEPTION : return exceptions->getByName(identifier);
      case SYM_SCOPE     : { 
                             CAoiBase *tmp = submodules->getByName(identifier);
                             if (!tmp) tmp = interfaces->getByName(identifier);
                             return tmp; 
                           }
      default            : assert(false);
    }
    
  return NULL;  
}

CAoiConstant::CAoiConstant(const char *name, CAoiType *type, CAoiConstBase *value, CAoiScope *parentScope, CAoiContext *context) : CAoiBase(context)

{
  assert(name!=NULL);
  assert(type!=NULL);
  assert(value!=NULL);
  
  this->name = name;
  this->type = type;
  this->value = value;
  this->flags = 0;
  this->parentScope = parentScope;
}  

CAoiParameter::CAoiParameter(CAoiType *type, CAoiDeclarator *decl, CAoiList *properties, CAoiContext *context) : CAoiDeclarator(decl, context)

{
  this->type = type;
  this->flags = 0;
  this->properties = properties;
  mergeProperties();
}

void CAoiDeclarator::merge(CAoiDeclarator *peer)

{
  refLevel += peer->refLevel; 
  for (int i=0;i<(peer->numBrackets);i++) 
    if (numBrackets<MAX_DIMENSION) 
      dimension[numBrackets++] = peer->dimension[i];
  if (peer->name)     
    name = peer->name;
  if (peer->bits)
    bits = peer->bits;
}

bool CAoiAliasType::canAssign(CAoiConstBase *cnst)

{
  return ref->canAssign(cnst);
}

CAoiAliasType::CAoiAliasType(CAoiType *ref, CAoiDeclarator *decl, CAoiScope *parentScope, CAoiContext *context) : CAoiType(decl->name, parentScope, context)

{
  this->ref = ref;
  this->numBrackets = decl->numBrackets;
  for (int i=0;i<numBrackets;i++)
    this->dimension[i] = decl->dimension[i]; 
}

CAoiOperation::CAoiOperation(CAoiScope *parentScope, CAoiType *returnType, const char *name, CAoiList *parameters, CAoiList *exceptions, CAoiList *properties, CAoiContext *context) : CAoiBase(context)

{
  this->parent = parentScope;
  this->returnType = returnType;
  this->name = name;
  this->parameters = parameters;
  this->exceptions = exceptions;
  this->flags = 0;
  this->fid = UNKNOWN;
  this->properties = properties;
  mergeProperties();
}

void CAoiOperation::mergeProperties()

{
  assert(properties);

  CAoiProperty *iterator = (CAoiProperty*)properties->getFirstElement();
  bool hasUUID = false;

  while (!iterator->isEndOfList())
    {
      bool ok = false;
    
      if (!strcasecmp(iterator->name, "oneway"))
        { 
          flags |= FLAG_ONEWAY; 
          ok = true;
        }
        
      if (!strcasecmp(iterator->name, "local"))
        {
          flags |= FLAG_LOCAL; 
          ok = true;
        }

      if (!strcasecmp(iterator->name, "ref"))
        {
          returnType = aoiFactory->buildPointerType(NULL, returnType, returnType->parentScope, returnType->context);
          ok = true;
        }
        
      if (!strcasecmp(iterator->name, "uuid"))
        {
          if (iterator->constValue->isScalar())
            {
              fid = ((CAoiConstInt*)iterator->constValue)->value;
              ok = true;
              hasUUID = true;
            } else semanticError(iterator->context, "identifier must be an integer");
        } 
        
      if (!strcasecmp(iterator->name, "kernelmsg"))
        {
          if (iterator->constValue->isScalar())
            {
              flags |= FLAG_KERNELMSG;
              fid = ((CAoiConstInt*)iterator->constValue)->value;
              ok = true;
            } else semanticError(iterator->context, "kernel message number must be an integer");
        }
      
      if (!ok)
        {
          semanticError(iterator->context, "bad attribute");
          properties->removeElement(iterator);
          iterator = (CAoiProperty*)properties->getFirstElement();
        } else iterator = (CAoiProperty*)iterator->next;    
    }
    
  if (hasUUID && (flags&FLAG_KERNELMSG))
    semanticError(context, "kernel messages cannot be assigned specific UUIDs");
}

void CAoiInterface::mergeProperties()

{
  assert(properties);

  CAoiProperty *iterator = (CAoiProperty*)properties->getFirstElement();
  while (!iterator->isEndOfList())
    {
      bool ok = false;
    
      if (!strcasecmp(iterator->name, "local"))
        {
          flags |= FLAG_LOCAL;
          ok = true;
        }
        
      if (!strcasecmp(iterator->name, "uuid"))
        {
          if (iterator->constValue->isScalar())
            {
              iid = ((CAoiConstInt*)iterator->constValue)->value;
              ok = true;
            } else semanticError(iterator->context, "identifier must be an integer");
        }
        
      if (!ok)
        {
          semanticError(iterator->context, "bad attribute");
          properties->removeElement(iterator);
          iterator = (CAoiProperty*)properties->getFirstElement();
        } else iterator = (CAoiProperty*)iterator->next;    
    }
}

void CAoiParameter::mergeProperties()

{
  bool isDCE = false;
  bool isString = false;
  bool usesItrans = false;
  bool usesOtrans = false;
  
  assert(properties);

  CAoiProperty *iterator = (CAoiProperty*)properties->getFirstElement();
  while (!iterator->isEndOfList())
    {
      bool ok = false;
    
      if (!strcasecmp(iterator->name, "dce_in"))
        {
          flags |= FLAG_IN; 
          isDCE = true;
          ok = true;
        }
        
      if (!strcasecmp(iterator->name, "dce_out"))
        {
          flags |= FLAG_OUT; 
          isDCE = true;
          ok = true;
        }
        
      if (!strcasecmp(iterator->name, "dce_string"))
        {
          if (!refLevel)
            semanticError(context, "string attribute is allowed for arrays only");
          isDCE = true;
          isString = true;
          ok = true;
        }
        
      if (!strcasecmp(iterator->name, "in"))
        {
          flags |= FLAG_IN;
          ok = true;
        }
        
      if (!strcasecmp(iterator->name, "out"))
        {
          flags |= FLAG_OUT;
          ok = true;
        }
        
      if (!strcasecmp(iterator->name, "inout"))
        {
          flags |= FLAG_IN + FLAG_OUT;
          ok = true;
        }

      if (!strcasecmp(iterator->name, "noxfer"))
        {
          flags |= FLAG_NOXFER;
          ok = true;
        }
        
      if (!strcasecmp(iterator->name, "prealloc"))
        {
          flags |= FLAG_PREALLOC;
          ok = true;
        }
        
      if (!strcasecmp(iterator->name, "ref"))
        {
          refLevel++;
          ok = true;
        }

      if (!strcasecmp(iterator->name, "max_value"))
        ok = true;

      if (!strcasecmp(iterator->name, "itrans"))
        {
          ok = true;
          usesItrans = true;
        }  

      if (!strcasecmp(iterator->name, "otrans"))
        {
          ok = true;
          usesOtrans = true;
        }  

      if (!strcasecmp(iterator->name, "handle"))
        ok = true;
        
#if 0        
      if (!strcasecmp(iterator->name, "map"))
        flags |= FLAG_MAP; else
      if (!strcasecmp(iterator->name, "grant"))
        flags |= FLAG_GRANT; else
      if (!strcasecmp(iterator->name, "writable"))
        flags |= FLAG_WRITABLE; else
      if (!strcasecmp(iterator->name, "nocache"))
        flags |= FLAG_NOCACHE; else
      if (!strcasecmp(iterator->name, "l1_only"))
        flags |= FLAG_L1_ONLY; else
      if (!strcasecmp(iterator->name, "all_caches"))
        flags |= FLAG_ALL_CACHES; else
      if (!strcasecmp(iterator->name, "follow"))
        follow = 1; else
      if (!strcasecmp(iterator->name, "readonly"))
        flags |= FLAG_READONLY; else
#endif

      if (!strcasecmp(iterator->name, "length_is"))
        { 
          if (!iterator->constValue || iterator->constValue->isScalar())
            {
              if (refLevel>0)
                {
                  ok = true;
                  isString = true;
                } else semanticError(context, "length_is attribute is allowed for arrays only");
            } else semanticError(iterator->context, "length must be scalar");
        }
        
      if (!ok)
        {
          semanticError(iterator->context, "bad attribute");
          properties->removeElement(iterator);
          iterator = (CAoiProperty*)properties->getFirstElement();
        } else iterator = (CAoiProperty*)iterator->next;    
    }

  if (isString)
    {
      type = new CAoiStringType(UNKNOWN, type, type->parentScope, context);
      refLevel--;
    }

  if (isDCE && (flags&FLAG_OUT))
    { 
      if (refLevel>0) refLevel--;
        else semanticError(context, "output parameter %s must be passed via reference", name);
    }
    
  if ((flags&FLAG_PREALLOC) && (flags&FLAG_IN) && (!(flags&FLAG_OUT)))
    semanticError(context, "cannot preallocate buffers for in values");

  if (usesItrans & (!(flags&FLAG_IN)))
    semanticError(context, "itrans can only be applied to input values");

  if (usesOtrans & (!(flags&FLAG_OUT)))
    semanticError(context, "otrans can only be applied to output values");

  while (refLevel>0)
    {
      type = aoiFactory->buildPointerType(NULL, type, type->parentScope, context);
      refLevel--;
    }
}

CAoiRootScope::CAoiRootScope() : CAoiScope(NULL, new CAoiContext("builtin", "root scope", 1, 1)) 

{ 
  CAoiContext *context = new CAoiContext("builtin", "default type", 1, 1);
  
  this->name = "root"; 
  this->scopedName = ""; 
  
  types->add(createAoiMachineWordType(this, context));
  types->add(createAoiSignedMachineWordType(this, context));
  types->add(createAoiThreadIDType(this, context));
  types->add(new CAoiIntegerType("#s8", 1, true, "CORBA_char", this, context));
  types->add(new CAoiIntegerType("#s16", 2, true, "CORBA_short", this, context));
  types->add(new CAoiIntegerType("#s32", 4, true, "CORBA_long", this, context));
  types->add(new CAoiIntegerType("#s64", 8, true, "CORBA_long_long", this, context));
  types->add(new CAoiIntegerType("#u8", 1, false, "CORBA_unsigned_char", this, context));
  types->add(new CAoiIntegerType("#u16", 2, false, "CORBA_unsigned_short", this, context));
  types->add(new CAoiIntegerType("#u32", 4, false, "CORBA_unsigned_long", this, context));
  types->add(new CAoiIntegerType("#u64", 8, false, "CORBA_unsigned_long_long", this, context));
  types->add(new CAoiIntegerType("#octet", 1, false, "CORBA_octet", this, context));
  types->add(new CAoiIntegerType("#wchar", 2, true, "CORBA_wchar", this, context));
  types->add(new CAoiIntegerType("#boolean", 1, false, "CORBA_boolean", this, context));
  types->add(new CAoiIntegerType("#void", 0, true, "void", this, context));
  types->add(new CAoiFloatType("#f32", 4, "CORBA_float", this, context));
  types->add(new CAoiFloatType("#f64", 8, "CORBA_double", this, context));
  types->add(new CAoiFloatType("#f96", 12, "CORBA_long_double", this, context));
  types->add(new CAoiCustomType("fpage", globals.word_size, this, context));
}

CAoiList *CAoiInterface::getAllSuperclasses()

{
  CAoiList *result = new CAoiList();

  result->add(new CAoiRef(this, this->context));

  forAll(superclasses, 
    {
      CAoiList *candidates = ((CAoiInterface*)(((CAoiRef*)item)->ref))->getAllSuperclasses();
      
      while (!candidates->isEmpty())
        {
          CAoiBase *thisCandidate = candidates->removeFirstElement();
          CAoiRef *thisRef = (CAoiRef*)thisCandidate;
          CAoiBase *thatCandidate = result->getFirstElement();
          bool foundThis = false;
          
          while (!thatCandidate->isEndOfList())
            {
              CAoiRef *thatRef = (CAoiRef*)thatCandidate;
              
              if (thisRef->ref == thatRef->ref)
                foundThis = true;
                
              thatCandidate = thatCandidate->next;
            }
            
          if (!foundThis)
            result->add(thisCandidate);
            
          thisCandidate = thisCandidate->next;
        }
    }
  );
  
  return result;
}

CAoiProperty *CAoiParameter::getProperty(const char *propName)

{
  forAll(properties, if (!strcmp(item->name, propName)) return (CAoiProperty*)item);
  return NULL;
}
