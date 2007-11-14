#include "chacmos.h"
#include "stdlib.h"
#include "interfaces/booter_server.h"
#include "booter.h"

IDL4_INLINE CORBA_long generic_implements_implementation(CORBA_Object _caller, const CORBA_long objID, const CORBA_long list_size, const CORBA_long *interface_list, idl4_server_environment *_env)

{
  if ((objID<0) || (objID>=(MAXTASKS+MAXBLOCKS)))
    return ENO;
    
  if (objID<MAXTASKS)
    {
      if (!task[objID].active)  
        return ENO;
        
      for (int i=0;i<list_size;i++) 
        if ((interface_list[i]!=GENERIC_API) &&
            (interface_list[i]!=TASK_API) &&
            (interface_list[i]!=CREATOR_API))
          return ENO;
   
    } else {
             if (!block[objID - MAXTASKS].active)
               return ENO;
   
             for (int i=0;i<list_size;i++) 
               if ((interface_list[i]!=GENERIC_API) &&
                   (interface_list[i]!=BLOCK_API))
                 return ENO;
           }    
    
  return EYES;
}

IDL4_PUBLISH_BOOTER_IMPLEMENTS(generic_implements_implementation);

IDL4_INLINE CORBA_long creator_can_create_implementation(CORBA_Object _caller, const CORBA_long objID, const CORBA_long list_size, const CORBA_long *interface_list, idl4_server_environment *_env)

{
  if ((objID<0) || (objID>=MAXTASKS) || (!task[objID].active))
    return ESUPP;
  
  for (int i=0;i<list_size;i++) 
    if ((interface_list[i]!=GENERIC_API) &&
        (interface_list[i]!=TASK_API))
      return ENO;
      
  return EYES;
}

IDL4_PUBLISH_BOOTER_CAN_CREATE(creator_can_create_implementation);
  
IDL4_INLINE CORBA_long creator_create_implementation(CORBA_Object _caller, const CORBA_long objID, const CORBA_long list_size, const CORBA_long *interface_list, l4_threadid_t *dsvrID, CORBA_long *dobjID, idl4_server_environment *_env)

{
  int x = 0;

  if ((objID<0) || (objID>=MAXTASKS) || (!task[objID].active))
    return ESUPP;

  for (int i=0;i<list_size;i++) 
    if ((interface_list[i]!=GENERIC_API) &&
        (interface_list[i]!=TASK_API))
      return ESUPP;
    
  while ((x<MAXTASKS) && (task[x].active))
    x++;
  if (x==MAXTASKS)
    return EFULL;
    
  task[x].active=1;
  task[x].root.svr=root.svr;
  task[x].root.obj=root.obj;
  task[x].memory.svr.raw=0;
  task[x].memory.obj=0;
  task[x].cmdline[0]=0;
  task[x].task_id.raw=0;
  task[x].owner=_caller;
  
  *dsvrID=l4_myself();
  *dobjID=x;

  return ESUCCESS;    
}

IDL4_PUBLISH_BOOTER_CREATE(creator_create_implementation);
  
IDL4_INLINE CORBA_long task_set_cmdline_implementation(CORBA_Object _caller, const CORBA_long objID, const CORBA_long size, const CORBA_char *cmdline, idl4_server_environment *_env)

{
  if ((objID<0) || (objID>=MAXTASKS) || (!task[objID].active))
    return ESUPP;

  if (_caller!=task[objID].owner)
    return EDENIED;
    
  if (size>=MAXLENGTH) 
    return EBADPARM;
    
  strncpy(task[objID].cmdline,cmdline,MAXLENGTH-1);
  
  #ifdef DEBUG
  printf("cmdline set to [%s] in tcb%d\n",task[objID].cmdline,objID);
  #endif
  
  return ESUCCESS;
}

IDL4_PUBLISH_BOOTER_SET_CMDLINE(task_set_cmdline_implementation);
  
IDL4_INLINE CORBA_long task_set_root_implementation(CORBA_Object _caller, CORBA_long objID, l4_threadid_t *dsvrID, CORBA_long dobjID, idl4_server_environment *_env)

{
  if ((objID<0) || (objID>=MAXTASKS) || (!task[objID].active))
    return ESUPP;

  if (_caller!=task[objID].owner)
    return EDENIED;

  task[objID].root.svr=*dsvrID;
  task[objID].root.obj=dobjID;
  
  #ifdef DEBUG
  printf("root set to <%X,%d> in tcb%d\n",dsvrID->raw,dobjID,objID);
  #endif
  
  return ESUCCESS;
}

IDL4_PUBLISH_BOOTER_SET_ROOT(task_set_root_implementation);
  
IDL4_INLINE CORBA_long task_set_memory_implementation(CORBA_Object _caller, CORBA_long objID, l4_threadid_t *psvrID, CORBA_long pobjID, idl4_server_environment *_env)

{
  if ((objID<0) || (objID>=MAXTASKS) || (!task[objID].active))
    return ESUPP;

  if (_caller!=task[objID].owner)
    return EDENIED;

  task[objID].memory.svr=*psvrID;
  task[objID].memory.obj=pobjID;

  #ifdef DEBUG
  printf("root set to <%X,%d> in tcb%d\n",psvrID->raw,pobjID,objID);
  #endif
  
  return ESUCCESS;
}

IDL4_PUBLISH_BOOTER_SET_MEMORY(task_set_memory_implementation);
  
IDL4_INLINE CORBA_long task_execute_implementation(CORBA_Object _caller, CORBA_long objID, l4_threadid_t *fsvrID, CORBA_long fobjID, idl4_server_environment *_env)

{
  handle_t file;

  if ((objID<0) || (objID>=MAXTASKS) || (!task[objID].active))
    return ESUPP;

  if (_caller!=task[objID].owner)
    return EDENIED;

  file.svr=*fsvrID;file.obj=fobjID;
  
  return elf_decode(objID,file);
}

IDL4_PUBLISH_BOOTER_EXECUTE(task_execute_implementation);
  
IDL4_INLINE CORBA_long task_kill_implementation(CORBA_Object _caller, CORBA_long objID, idl4_server_environment *_env)

{
  if ((objID<0) || (objID>=MAXTASKS) || (!task[objID].active))
    return ESUPP;

  if ((_caller!=task[objID].owner) && (_caller!=task[objID].task_id))
    return EDENIED;
    
  strip(objID);
  
  l4_threadid_t candidate, nil;
  candidate=task[objID].task_id;nil.raw=0;
  
  l4_task_new(candidate, 255, 0, 0, nil);
  
  return ESUCCESS;
}

IDL4_PUBLISH_BOOTER_KILL(task_kill_implementation);
  
IDL4_INLINE CORBA_long task_get_cmdline_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long *size, CORBA_char **cmdline, idl4_server_environment *_env)

{
  if ((objID<0) || (objID>=MAXTASKS) || (!task[objID].active))
    return ESUPP;
    
  *cmdline=&task[objID].cmdline[0];
  *size=strlen(task[objID].cmdline)+1;
  
  return ESUCCESS;
}

IDL4_PUBLISH_BOOTER_GET_CMDLINE(task_get_cmdline_implementation);

IDL4_INLINE CORBA_long task_get_root_implementation(CORBA_Object _caller, CORBA_long objID, l4_threadid_t *dsvrID, CORBA_long *dobjID, idl4_server_environment *_env)

{
  if ((objID<0) || (objID>=MAXTASKS) || (!task[objID].active))
    return ESUPP;
    
  *dsvrID=task[objID].root.svr;
  *dobjID=task[objID].root.obj;
  
  return ESUCCESS;
}

IDL4_PUBLISH_BOOTER_GET_ROOT(task_get_root_implementation);
  
IDL4_INLINE CORBA_long task_get_threadid_implementation(CORBA_Object _caller, CORBA_long objID, l4_threadid_t *threadID, idl4_server_environment *_env)

{
  if ((objID<0) || (objID>=MAXTASKS) || (!task[objID].active))
    return ESUPP;
    
  *threadID=task[objID].task_id;
  
  return ESUCCESS;
}

IDL4_PUBLISH_BOOTER_GET_THREADID(task_get_threadid_implementation);
  
IDL4_INLINE CORBA_long task_get_memory_implementation(CORBA_Object _caller, CORBA_long objID, l4_threadid_t *psvrID, CORBA_long *pobjID, idl4_server_environment *_env)

{
  if ((objID<0) || (objID>=MAXTASKS) || (!task[objID].active))
    return ESUPP;
    
  *psvrID=task[objID].memory.svr;
  *pobjID=task[objID].memory.obj;
  
  return ESUCCESS;
}

IDL4_PUBLISH_BOOTER_GET_MEMORY(task_get_memory_implementation);
  
IDL4_INLINE CORBA_long block_get_capacity_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long *blockcnt, idl4_server_environment *_env)

{ 
  int devnr = objID-MAXTASKS;
  
  if ((devnr<0) || (devnr>=MAXBLOCKS) || (!block[devnr].active))
    return ESUPP;
    
  *blockcnt=block[devnr].capacity;
  
  return ESUCCESS;
}  

IDL4_PUBLISH_BOOTER_GET_CAPACITY(block_get_capacity_implementation);

IDL4_INLINE CORBA_long block_get_blocksize_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long *size, idl4_server_environment *_env)

{ 
  int devnr = objID-MAXTASKS;
  
  if ((devnr<0) || (devnr>=MAXBLOCKS) || (!block[devnr].active))
    return ESUPP;
    
  *size=block[devnr].blocksize;
  
  return ESUCCESS;
}  

IDL4_PUBLISH_BOOTER_GET_BLOCKSIZE(block_get_blocksize_implementation);

IDL4_INLINE CORBA_long block_read_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long blocknr, CORBA_long count, CORBA_long *size, CORBA_char **data, idl4_server_environment *_env)

{ 
  int devnr = objID-MAXTASKS;
  
  if ((devnr<0) || (devnr>=MAXBLOCKS) || (!block[devnr].active))
    return ESUPP;

  if (count<1)
    return EBADPARM;

  if ((blocknr<0) || ((blocknr+count)>=(block[devnr].capacity-1)))
    return EEOF;
    
  *size=block[devnr].blocksize*count;
  *data=(char*)(block[devnr].phys_addr + blocknr * block[devnr].blocksize);

  return ESUCCESS;
}  

IDL4_PUBLISH_BOOTER_READ(block_read_implementation);

IDL4_INLINE CORBA_long block_write_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long blocknr, CORBA_long count, CORBA_long size, CORBA_char *data, idl4_server_environment *_env)

{ 
  int devnr = objID-MAXTASKS;
  
  if ((devnr<0) || (devnr>=MAXBLOCKS) || (!block[devnr].active))
    return ESUPP;

  if ((count<1) || (size!=(count*block[devnr].blocksize)))
    return EBADPARM;

  if ((blocknr<0) || ((blocknr+count)>=(block[devnr].capacity-1)))
    return EEOF;

  int *dest = (int*)(block[devnr].phys_addr + blocknr * block[devnr].blocksize);
  int *src = (int*)data;
  
  size>>=2;
  while (size--) 
    *dest++=*src++;
  
  return ESUCCESS;
}  

IDL4_PUBLISH_BOOTER_WRITE(block_write_implementation);

IDL4_INLINE CORBA_long block_set_capacity_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long blockcnt, idl4_server_environment *_env)

{
  return ESUPP;
}  

IDL4_PUBLISH_BOOTER_SET_CAPACITY(block_set_capacity_implementation);
