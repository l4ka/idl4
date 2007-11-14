#include "chacmos.h"
#include "stdlib.h"
#include "interfaces/memsvr_server.h"
#include "memsvr.h"

IDL4_INLINE CORBA_long generic_implements_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long list_size, CORBA_long *interface_list, idl4_server_environment *_env)
        
{
  int i;
  
  if (objID==DEFAULT_OBJECT)
    {
      for (i=0;i<list_size;i++) 
        if ((interface_list[i]!=GENERIC_API) &&
            (interface_list[i]!=CREATOR_API))
          return ENO;
          
      return EYES;    
    } else       
      
  if ((objID>=0) && (objID<MAXTASKS) && (space[objID].thread_id.raw!=0xFFFFFFFF))
    {
      for (i=0;i<list_size;i++) 
        if ((interface_list[i]!=GENERIC_API) &&
            (interface_list[i]!=MEMORY_API))
          return ENO;
          
      return EYES;   
    }
        
  return ENO;
}

IDL4_PUBLISH_MEMSVR_IMPLEMENTS(generic_implements_implementation);
  
IDL4_INLINE CORBA_long creator_can_create_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long list_size, CORBA_long *interface_list, idl4_server_environment *_env)

{
  int i;
  
  if (objID==DEFAULT_OBJECT)
    {
      for (i=0;i<list_size;i++) 
        if ((interface_list[i]!=GENERIC_API) &&
            (interface_list[i]!=MEMORY_API))
          return ENO;
          
      return EYES; 
    }  
      
  return ENO;
}

IDL4_PUBLISH_MEMSVR_CAN_CREATE(creator_can_create_implementation);
  
IDL4_INLINE CORBA_long creator_create_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long list_size, CORBA_long *interface_list, l4_threadid_t *dsvrID, CORBA_long *dobjID, idl4_server_environment *_env)

{
  if (objID==DEFAULT_OBJECT)
    {
      int x=2,i;
      l4_threadid_t myself = l4_myself();

      // *** We cannot create anything other that memory objects

      for (i=0;i<list_size;i++) 
        if ((interface_list[i]!=GENERIC_API) &&
            (interface_list[i]!=MEMORY_API))
          return ESUPP;
      
      // *** The user wants some memory, so we set up a descriptor for it
      
      while ((x<MAXTASKS) && (space[x].thread_id.raw!=0xFFFFFFFF)) x++;
      if (x==MAXTASKS) return EFULL;
      *dsvrID=myself;
      *dobjID=x;
      space[x].thread_id.raw=0;
      
      return ESUCCESS;
    }  
    
  return ESUPP;
}

IDL4_PUBLISH_MEMSVR_CREATE(creator_create_implementation);
  
IDL4_INLINE CORBA_long memory_get_pagerid_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long *pagerID, idl4_server_environment *_env)

{
  if ((objID>=2) && (objID<MAXTASKS))
    if (space[objID].thread_id.raw!=0xFFFFFFFF)
      {
        l4_threadid_t pager = l4_myself();
        pager.id.thread=1;
        *pagerID=pager.raw;
        return ESUCCESS;
      }  
  return ESUPP;
}

IDL4_PUBLISH_MEMSVR_GET_PAGERID(memory_get_pagerid_implementation);
  
IDL4_INLINE CORBA_long memory_request_physical_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long fpage, idl4_server_environment *_env)

{
  return ESUPP;
}

IDL4_PUBLISH_MEMSVR_REQUEST_PHYSICAL(memory_request_physical_implementation);
  
IDL4_INLINE CORBA_long memory_unmap_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long fpage, idl4_server_environment *_env)

{
  // the pages should be removed
  return ESUCCESS;
}

IDL4_PUBLISH_MEMSVR_UNMAP(memory_unmap_implementation);
  
IDL4_INLINE CORBA_long memory_attach_implementation(CORBA_Object _caller, CORBA_long objID, l4_threadid_t *taskID, idl4_server_environment *_env)

{
  if ((objID>=2) && (objID<MAXTASKS))
    {
      if (space[objID].thread_id.raw==0xFFFFFFFF)
        return ESUPP;
      if (space[objID].thread_id.raw!=0)
        return ELOCKED;  
      space[objID].thread_id=*taskID;
      space[objID].boss_id=_caller;
      return ESUCCESS;
    }
  return ESUPP;
}

IDL4_PUBLISH_MEMSVR_ATTACH(memory_attach_implementation);
  
IDL4_INLINE CORBA_long memory_set_maxpages_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long maxpages, idl4_server_environment *_env)

{
  if ((objID>=2) && (objID<MAXTASKS))
    {
      if (space[objID].thread_id.raw==0xFFFFFFFF)
        return ESUPP;
      if ((space[objID].thread_id.raw!=0) && (space[objID].boss_id!=_caller))
        return ELOCKED;
      space[objID].maxpages=maxpages;
      return ESUCCESS;
    }    
  return ESUPP;
}

IDL4_PUBLISH_MEMSVR_SET_MAXPAGES(memory_set_maxpages_implementation);
  
IDL4_INLINE CORBA_long memory_get_mapped_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long fpage, CORBA_long *pages, idl4_server_environment *_env)

{
  return ESUPP;
}
  
IDL4_PUBLISH_MEMSVR_GET_MAPPED(memory_get_mapped_implementation);
