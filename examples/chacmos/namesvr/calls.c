#include "chacmos.h"
#include "stdlib.h"
#include "interfaces/namesvr_server.h"
#include "namesvr.h"

IDL4_INLINE CORBA_long generic_implements_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long list_size, CORBA_long *interface_list, idl4_server_environment *_env)

{
  if (objID>=MAXDIRS)
    return ENO;

  #ifdef DEBUG
  printf("implements of obj %d\n",objID);
  printf("list: ");for (int i=0;i<list_size;i++) printf("%X ",interface_list[i]);
  printf("\n"); 
  #endif
    
  if (objID==DEFAULT_OBJECT)
    {
      for (int i=0;i<list_size;i++) 
        if ((interface_list[i]!=GENERIC_API) &&
            (interface_list[i]!=CREATOR_API))
          return ENO;
          
      return EYES;    
    }
  
  if (directory[objID].active) 
    {
      for (int i=0;i<list_size;i++) 
        if ((interface_list[i]!=GENERIC_API) &&
            (interface_list[i]!=DIRECTORY_API) &&
            (interface_list[i]!=FILE_API) &&
            (interface_list[i]!=CREATOR_API))
          return ENO;

      return EYES;    
    }
    
  return ENO;
}

IDL4_PUBLISH_NAMESVR_IMPLEMENTS(generic_implements_implementation);

IDL4_INLINE CORBA_long directory_resolve_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long size, CORBA_char *name, l4_threadid_t *dsvrID, CORBA_long *dobjID, idl4_server_environment *_env)

{
  int pos;

  if (size>256) size=256;
/**/  if (size<0) enter_kdebug("too small");
  name[size]=0;

  #ifdef DEBUG
  printf("%X resolved %s[%d] in %d",_caller.raw,name,size,objID);
  #endif

  if ((objID>=MAXDIRS) || (!directory[objID].active))
    return ESUPP;
  
  #ifdef DEBUG
  printf(", which is active");
  #endif
    
  pos=directory[objID].first_child;
  while ((pos>=0) && (strcmp(nameslot[pos].ascii,name)))
    pos=nameslot[pos].next_peer;
      
  if (pos<0)    
    return ENOTFOUND;
    
  #ifdef DEBUG
  printf(", to <%X,%d>\n",nameslot[pos].svrID.raw,nameslot[pos].objID);
  #endif  
    
  *dsvrID=nameslot[pos].svrID;
  *dobjID=nameslot[pos].objID;
  return ESUCCESS;
}

IDL4_PUBLISH_NAMESVR_RESOLVE(directory_resolve_implementation);

IDL4_INLINE CORBA_long directory_link_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long size, CORBA_char *name, l4_threadid_t *dsvrID, CORBA_long dobjID, idl4_server_environment *_env)

{
  int pos=0;

  if (size>256) size=256;
  if (size<0) enter_kdebug("small2");
  name[size]=0;
  
  #ifdef DEBUG
  printf("%X is linking %s to <%X,%d> in object %d...\n",_caller.raw,name,dsvrID->raw,dobjID,objID);
  #endif  
  
  if ((objID>=MAXDIRS) || (!directory[objID].active))
    return ESUPP;
    
  if (size>=MAXLENGTH)
    return EBADPARM;
    
  pos=directory[objID].first_child;
  while ((pos>=0) && (strcmp(nameslot[pos].ascii,name)))
    pos=nameslot[pos].next_peer;
  if (pos>=0)
    return EEXISTS;

  // --- scan for an empty name slot
    
  pos=0;  
  while ((pos<MAXNAMES) && (nameslot[pos].active))
    pos++;
    
  if (pos>=MAXNAMES)
    return EFULL;

  // --- enter the name  
  
  nameslot[pos].active=1;
  nameslot[pos].next_peer=directory[objID].first_child;
  strncpy(nameslot[pos].ascii,name,size);
  nameslot[pos].ascii[size+1]=0;
  directory[objID].first_child=pos;
  nameslot[pos].svrID=*dsvrID;
  nameslot[pos].objID=dobjID;
  
  return ESUCCESS;
}

IDL4_PUBLISH_NAMESVR_LINK(directory_link_implementation);

IDL4_INLINE CORBA_long directory_unlink_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long size, CORBA_char *name, idl4_server_environment *_env)

{
  int pos,trail;

  if (size>256) size=256;
  name[size]=0;
  
  if ((objID>=MAXDIRS) || (!directory[objID].active))
    return ESUPP;

  trail=-1;pos=directory[objID].first_child;
  while (pos>=0)
    if (strcmp(nameslot[pos].ascii,name))
      { 
        trail=pos;
        pos=nameslot[pos].next_peer;
      }  
  if (pos<0) 
    return ENOTFOUND;
  
  if (trail>=0)
    {
      nameslot[trail].next_peer=nameslot[pos].next_peer;
    } else directory[objID].first_child=nameslot[pos].next_peer;             
    
  nameslot[pos].active=0;  
  return ESUCCESS;
}

IDL4_PUBLISH_NAMESVR_UNLINK(directory_unlink_implementation);

IDL4_INLINE CORBA_long creator_can_create_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long list_size, CORBA_long *interface_list, idl4_server_environment *_env)

{
  if ((objID<0) || (objID>=MAXDIRS) || ((!directory[objID].active) && (objID!=DEFAULT_OBJECT)))
    return ESUPP;

  for (int i=0;i<list_size;i++) 
    if ((interface_list[i]!=GENERIC_API) &&
        (interface_list[i]!=FILE_API) &&
        (interface_list[i]!=DIRECTORY_API))
      return ENO;
          
  return EYES;    
}

IDL4_PUBLISH_NAMESVR_CAN_CREATE(creator_can_create_implementation);

IDL4_INLINE CORBA_long creator_create_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long list_size, CORBA_long *interface_list, l4_threadid_t *dsvrID, CORBA_long *dobjID, idl4_server_environment *_env)

{
  int pos=1;
  
  if ((objID<0) || (objID>=MAXDIRS) || ((!directory[objID].active) && (objID!=DEFAULT_OBJECT)))
    return ESUPP;

  // *** We can just do directories, nothing else

  for (int i=0;i<list_size;i++) 
    if ((interface_list[i]!=GENERIC_API) &&
        (interface_list[i]!=FILE_API) &&
        (interface_list[i]!=DIRECTORY_API))
      return ESUPP;
    
  while ((pos<MAXDIRS) && (directory[pos].active))
    pos++;
    
  if (pos>=MAXDIRS)
    return EFULL;
    
  directory[pos].active=1;
  directory[pos].first_child=-1;
  *dsvrID=mytaskid;
  *dobjID=pos;
  return ESUCCESS;
}

IDL4_PUBLISH_NAMESVR_CREATE(creator_create_implementation);

IDL4_INLINE CORBA_long file_open_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long mode, idl4_server_environment *_env)

{
  int i, pos, ptr;

  #ifdef DEBUG
  printf("Namesvr: %X is opening %d, ",_caller.raw,objID);
  #endif

  if ((objID<0) || (objID>=MAXDIRS) || (!directory[objID].active))
    return ESUPP;

  for (i=0;i<MAXFILES;i++)
    if ((file[i].threadID==_caller) && (file[i].objID==objID) && (file[i].active))
      return ENOTOPEN;
      
  pos=0;
  while ((pos<MAXFILES) && (file[pos].active))
    pos++;
  if (pos==MAXFILES) 
    return EFULL;  
    
  file[pos].active=1;
  file[pos].threadID=_caller;
  file[pos].objID=objID;
  file[pos].pos=0;
  i=directory[objID].first_child;
  ptr=0;
  while (i>=0)
    {
      if (ptr)
        file[pos].data[ptr++]=0;
      strncpy(&file[pos].data[ptr],nameslot[i].ascii,(MAXDATA-ptr-1));
      ptr+=strlen(nameslot[i].ascii);
      if (ptr>=(MAXDATA-1)) 
        ptr=MAXDATA-2;
      i=nameslot[i].next_peer;  
    }
  file[pos].size=ptr;
  
  #ifdef DEBUG
  printf("dirsize is %d\n",ptr);    
  #endif 
  
  return ESUCCESS;
}

IDL4_PUBLISH_NAMESVR_OPEN(file_open_implementation);

IDL4_INLINE CORBA_long file_close_implementation(CORBA_Object _caller, CORBA_long objID, idl4_server_environment *_env)

{
  int pos=0;

  if ((objID<0) || (objID>=MAXDIRS) || (!directory[objID].active))
    return ESUPP;

  while ((pos<MAXFILES) && ((file[pos].threadID!=_caller) || (file[pos].objID!=objID)))
    pos++;
    
  if (pos==MAXFILES)
    return ENOTOPEN;  
    
  file[pos].active=0;  
  return ESUCCESS;
}

IDL4_PUBLISH_NAMESVR_CLOSE(file_close_implementation);

IDL4_INLINE CORBA_long file_read_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long length, CORBA_long *size, CORBA_char **data, idl4_server_environment *_env)

{
  int pos=0;

  #ifdef DEBUG
  printf("Namesvr: %X is reading %d bytes of %d\n",_caller.raw,length,objID);
  #endif
  
  if ((objID<0) || (objID>=MAXDIRS) || (!directory[objID].active))
    return ESUPP;

  while ((pos<MAXFILES) && ((file[pos].threadID!=_caller) || (file[pos].objID!=objID)))
    pos++;
    
  if (pos==MAXFILES)
    return ENOTOPEN;  

  if (file[pos].pos==file[pos].size)
    return EEOF;
    
  *data=(char*)&file[pos].data[file[pos].pos];
  *size=file[pos].size-file[pos].pos;
  if (length<*size) 
    *size=length;
  file[pos].pos+=*size;
  return *size;
}

IDL4_PUBLISH_NAMESVR_READ(file_read_implementation);

IDL4_INLINE CORBA_long file_write_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long length, CORBA_long size, CORBA_char *data, idl4_server_environment *_env)

{
  int pos=0;
  
  if ((objID<0) || (objID>=MAXDIRS) || (!directory[objID].active))
    return ESUPP;

  while ((pos<MAXFILES) && ((file[pos].threadID!=_caller) || (file[pos].objID!=objID)))
    pos++;
    
  if (pos==MAXFILES)
    return ENOTOPEN;  
    
  return EDENIED;  
}

IDL4_PUBLISH_NAMESVR_WRITE(file_write_implementation);

IDL4_INLINE CORBA_long file_seek_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long pos, idl4_server_environment *_env)

{
  int i=0;

  #ifdef DEBUG
  printf("Namesvr: %X is seeking %d to %d\n",_caller.raw,objID,pos);
  #endif
  
  if ((objID<0) || (objID>=MAXDIRS) || (!directory[objID].active))
    return ESUPP;

  while ((i<MAXFILES) && ((file[i].threadID!=_caller) || (file[i].objID!=objID)))
    i++;
    
  if (i==MAXFILES)
    return ENOTOPEN;  
    
  file[i].pos=(unsigned int)pos;
  if (file[i].pos>file[i].size)
    file[i].pos=file[i].size;
  
  return ESUCCESS;    
}

IDL4_PUBLISH_NAMESVR_SEEK(file_seek_implementation);

IDL4_INLINE CORBA_long file_tell_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long *pos, idl4_server_environment *_env)

{
  int i=0;
  
  if ((objID<0) || (objID>=MAXDIRS) || (!directory[objID].active))
    return ESUPP;

  while ((i<MAXFILES) && ((file[i].threadID!=_caller) || (file[i].objID!=objID)))
    i++;
    
  if (i==MAXFILES)
    return ENOTOPEN;  

  *pos=file[i].pos;
  return ESUCCESS;    
}

IDL4_PUBLISH_NAMESVR_TELL(file_tell_implementation);

IDL4_INLINE CORBA_long file_delete_implementation(CORBA_Object _caller, CORBA_long objID, idl4_server_environment *_env)

{
  int pos;

  if ((objID>=MAXDIRS) || (!directory[objID].active))
    return ESUPP;

  directory[objID].active=0;
  pos=directory[objID].first_child;
  while (pos>=0)
    { 
      nameslot[pos].active=0;
      pos=nameslot[pos].next_peer;
    }  
    
  for (pos=0;pos<MAXFILES;pos++)
    if (file[pos].objID==objID)
      file[pos].active=0;  
    
  return ESUCCESS;  
}

IDL4_PUBLISH_NAMESVR_DELETE(file_delete_implementation);
