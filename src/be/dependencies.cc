#include "be.h"

#define dprintln(a...) do { if (debug_mode & DEBUG_REORDER) println(a); } while (0)

void CBEDependencyList::registerParam(CBEParameter *param)

{
  dprintln("Registered parameter %s", param->aoi->name);

  this->paramDeps = new CBEParamDep(param, this->paramDeps);
}

void CBEDependencyList::registerDependency(CBEParameter *param, CBEParameter *dependency)

{
  CBEParamDep *parIter = this->paramDeps;
  
  dprintln("Registered dependency from %s to %s", param->aoi->name, dependency->aoi->name);
  
  while (parIter!=NULL)
    if (parIter->param == param)
      {
        CBEDependency *depIter = parIter->dependencies;
        while (depIter!=NULL)
          if (depIter->to == dependency)
            return; /* is a duplicate */
            else depIter = depIter->next;
            
        parIter->dependencies = new CBEDependency(dependency, parIter->dependencies);
        
      } else parIter = parIter->next;
      
  panic("registerDependency without registerParam");
}

bool CBEParamDep::hasUnresolvedDependencies()

{
  CBEDependency *depIter = dependencies;

  while (depIter!=NULL)
    if (depIter->done)
      depIter = depIter->next;
      else return true;

  return false;
}

void CBEDependencyList::removeAllDependenciesTo(CBEParameter *param)

{
  for (CBEParamDep *parIter = paramDeps; parIter!=NULL; parIter = parIter->next)
    if (!parIter->done)
      for (CBEDependency *depIter = parIter->dependencies; depIter!=NULL; depIter = depIter->next)
        if (depIter->to == param)
          depIter->done = true;
}

bool CBEDependencyList::commit()

{
  bool moreWorkToDo, madeProgress;
  int prio = 0;

  dprintln("Committing");
  
  do {
       CBEParamDep *parIter = paramDeps;
  
       moreWorkToDo = false;
       madeProgress = false;
       
       while (parIter!=NULL)
         {
           if (!parIter->done && !parIter->hasUnresolvedDependencies())
             {
               removeAllDependenciesTo(parIter->param);
               parIter->param->assignPriority(++prio);
               dprintln("Assigned prio %d to parameter %s", prio, parIter->param->aoi->name);
               parIter->done = true;
               madeProgress = true;
             }
             
           if (!parIter->done)  
             moreWorkToDo = true;

           parIter = parIter->next;
         }
            
       if (moreWorkToDo && !madeProgress)
         return false; /* circular dependency */
       
     } while (moreWorkToDo);
     
  return true;
}
