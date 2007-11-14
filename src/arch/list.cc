#include <string.h>

#include "ms.h"

void CMSList::dump(const char *title)

{
  if (!isEmpty())
    {
      indent(+1);
      println(title);
      indent(+1);
      
      CMSBase *iterator = getFirstElement();
      while (!iterator->isEndOfList())
        {
          iterator->dump();
          iterator = iterator->next;
        }
      
      indent(-2);
    } 
} 

void CMSBase::add(CMSBase *newElement)

{
  assert(newElement!=NULL);
  assert(newElement->prev==NULL);
  assert(newElement->next==NULL);
  
  newElement->next = this;
  newElement->prev = this->prev;
  (this->prev)->next = newElement;
  this->prev = newElement;
}

void CMSList::addWithPrio(CMSBase *newElement, int prio)

{
  CMSBase *iterator = getFirstElement();
  while (!iterator->isEndOfList())
    if (iterator->prio<=prio)
      iterator = iterator->next;
      else break;
      
  iterator->add(newElement);
  newElement->prio = prio;
}

CMSBase *CMSList::removeFirstElement()

{
  assert(!isEmpty());
  
  CMSBase *elem = this->next;
  this->next = (this->next)->next;
  (this->next)->prev = this;
  elem->next = NULL;
  elem->prev = NULL;
  
  return elem;
}

CMSBase *CMSList::removeElement(CMSBase *element)

{
  CMSBase *iterator = getFirstElement();
  
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

CMSBase *CMSList::getFirstElementWithPrio(int prio)

{
  CMSBase *iterator = getFirstElement();
  
  assert(iterator!=NULL);
  
  while (!iterator->isEndOfList())
    if (iterator->prio != prio)
      iterator = iterator->next;
      else return iterator;
    
  return NULL;
}

bool CMSList::hasPrioOrAbove(int prio)

{
  CMSBase *iterator = getFirstElement();
  
  assert(iterator!=NULL);
  
  while (!iterator->isEndOfList())
    if (iterator->prio < prio)
      iterator = iterator->next;
      else return true;
    
  return false;
}

void CMSList::merge(CMSList *list)

{
  while (!list->isEmpty())
    this->add(list->removeFirstElement());
}    

CMSBase *CMSList::getByName(const char *identifier)

{
  if (!isEmpty())
    {
      CMSBase *iterator = getFirstElement();
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
