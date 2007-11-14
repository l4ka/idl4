#include "chacmos.h"
#include "stdlib.h"
#include "interfaces/filesvr_server.h"
#include "filesvr.h"
#include "dir.h"

fcb_t fcbTable[10];
bool mounted = false;

IDL4_INLINE CORBA_long generic_implements_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long list_size, CORBA_long *interface_list, idl4_server_environment *_env)

{
  // Test Interfaces
  int interfaceID=0;
  int found =1;
  int count = 0;
  int size = (int) list_size;
  
  #ifdef DEBUG
  printf("size is : %X \n",size);
  printf("list_size is : %X \n",list_size);
  int *some=interface_list;
  printf("the list is: ");for (int i=0;i<4;i++) printf("%X ",*some++);
  printf("\n");
  #endif
  
  while ((found == 1) && (count < size)){
    interfaceID = *(interface_list+count);
    
    #ifdef DEBUG
    printf("InterfaceID : %X \n",interfaceID);
    #endif
    
   if ((interfaceID==GENERIC_API) || ((objID==DEFAULT_OBJECT) && (interfaceID==FILESYSTEM_API)) ||
       ((mounted) && ((interfaceID==DIRECTORY_API) || (interfaceID==FILE_API)))){
   }
   else{
     found = 0;
   }

   // if this is not a directory, it will not support the directory api
   if ((found) && (interfaceID==DIRECTORY_API) && ((get_mode(objID)&0x3f000)!=0x4000))
     found=0;
   
   count++;
  }
  
  if (found ==1)
    return EYES;                                                                          

  return ENO;
}

IDL4_PUBLISH_FILESVR_IMPLEMENTS(generic_implements_implementation);

IDL4_INLINE CORBA_long directory_resolve_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long size, CORBA_char *name, l4_threadid_t *dsvrID, CORBA_long *dobjID, idl4_server_environment *_env)

{
  if (!mounted) return ESUPP;
  
  int file_length = get_length(objID); // length of the file
  char *buffer;
  
  read(objID,0,file_length,&buffer);

  // parsing buffer
  ext2_dir_entry *parse;
  parse = (ext2_dir_entry*)buffer;
  int count = 0;
  char str[80];
  name [size]=0;
  while (count <file_length){
    strncpy(str,parse->name,parse->name_length&0xff);

    if(!strcmp(str,name)){
      
      *dsvrID=l4_myself();
      *dobjID=parse->inode;
      return ESUCCESS;
    }
    count += (int)parse->rec_length;
    parse=(ext2_dir_entry*)(((char*)parse)+parse->rec_length); // really fine pointers
  }

  return ENOTFOUND;
}

IDL4_PUBLISH_FILESVR_RESOLVE(directory_resolve_implementation);

IDL4_INLINE CORBA_long directory_link_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long size, CORBA_char *name, l4_threadid_t *dsvrID, CORBA_long dobjID, idl4_server_environment *_env)

{
  return ESUPP;
}

IDL4_PUBLISH_FILESVR_LINK(directory_link_implementation);

IDL4_INLINE CORBA_long directory_unlink_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long size, CORBA_char *name, idl4_server_environment *_env)

{
  return ESUPP;
}

IDL4_PUBLISH_FILESVR_UNLINK(directory_unlink_implementation);

IDL4_INLINE CORBA_long file_open_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long mode, idl4_server_environment *_env)

{
  int inactive_fcb=0;
  int fcbNummer=0;
  int temp =0;
  int inode_type;

  if (!mounted) return ESUPP;

  // test objID (directory or regular file)
  temp = get_mode(objID);

  inode_type = temp & 0x3f000; //Mask last 4 Octals
  if (inode_type==0x04000){
    inode_type=DIRECTORY;
  }
  else{
    if (inode_type==0x08000){
      inode_type=FILE;
    }
    else{
      return ESUPP;
    }
  }
  
  //  printf("Finding inactive FCB \n");
  // finding inactive
  while(fcbNummer<(MAXFCB-1)){

    if (fcbTable[fcbNummer].active==0){
      inactive_fcb=1;
      break;
    }
    else
      fcbNummer++;
  }
  
    // if exist inactive fcb - set fcb
  if (inactive_fcb==1){
    //    printf("the inactive fcbnumber is : %X\n", fcbNummer);
    int file_length = get_length(objID); // length of the file
    fcbTable[fcbNummer].taskID=_caller;
    fcbTable[fcbNummer].objID=(int)objID;
    fcbTable[fcbNummer].position=0;
    fcbTable[fcbNummer].mode=(int)mode;
    fcbTable[fcbNummer].inode_type=inode_type;
    fcbTable[fcbNummer].active=1;
    fcbTable[fcbNummer].file_length=file_length;
    if (inode_type==DIRECTORY){
      char *buffer;
      read(objID,0,file_length,&buffer);
      // parsing buffer
      ext2_dir_entry *parse;
      parse = (ext2_dir_entry*)buffer;
      int count = 0;
      char *dirptr = &fcbTable[fcbNummer].dir_buffer[0];
      int count_length=0;
      while (count <file_length){
	strncpy(dirptr,parse->name,parse->name_length&0xFF);
	count_length += (parse->name_length&0xFF)+1;
	count += (int)parse->rec_length;
	dirptr+=(parse->name_length&0xFF)+1;
	parse=(ext2_dir_entry*)(((char*)parse)+parse->rec_length); // really fine pointer arithmetic
      }
      fcbTable[fcbNummer].file_length=count_length-1; //setting the parsed size minus one zero
    }
  }
  else
    return EAGAIN;

  return ESUCCESS; // file is open
}

IDL4_PUBLISH_FILESVR_OPEN(file_open_implementation);

IDL4_INLINE CORBA_long file_close_implementation(CORBA_Object _caller, CORBA_long objID, idl4_server_environment *_env)

{
  if (!mounted) return ESUPP;

  // test - is this object open
  int objFound =0;
  int fcbNummer=0;
  while ((objFound==0)&&(fcbNummer<MAXFCB-1)){
    if ((fcbTable[fcbNummer].taskID==_caller)&&(fcbTable[fcbNummer].objID==(int)objID)
	&&(fcbTable[fcbNummer].active==1)){
      fcbTable[fcbNummer].active=0;  // inactive fcb
      objFound=1;
    }
    else
      fcbNummer++;
  }
  if (objFound==1)
    return ESUCCESS;
  else
    return ENOTOPEN;
}

IDL4_PUBLISH_FILESVR_CLOSE(file_close_implementation);

IDL4_INLINE CORBA_long file_read_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long length, CORBA_long *size, CORBA_char **data, idl4_server_environment *_env)

{
  if (!mounted) return ESUPP;

  //find fcb for the objID
  int objFound = 0;
  int fcbNummer= 0;
  while((objFound==0)&&(fcbNummer<MAXFCB-1)){
    if ((fcbTable[fcbNummer].taskID==_caller)&&(fcbTable[fcbNummer].objID==(int)objID)
	&&(fcbTable[fcbNummer].active==1)){
      objFound=1;
    }
    else
      fcbNummer++;
  }
  if (objFound==0)
    return ENOTOPEN;

  int fileSize = fcbTable[fcbNummer].file_length;
  // check pointer and calculate remainder
  int filePointer = fcbTable[fcbNummer].position;
  int remainingSize = fileSize - filePointer;

  if (remainingSize == 0)
     return EEOF;

  int readingBytes = 0;
  if (remainingSize>=(int)length)
    readingBytes = (int) length;
  else
    readingBytes = remainingSize;
  

  // return size and data pointer  
  char *buffer;
  // check Inode is a Directory
  if (fcbTable[fcbNummer].inode_type==DIRECTORY){
    buffer=&fcbTable[fcbNummer].dir_buffer[filePointer];
  }
  else{
  //printf("Start reading file\n");
  read(fcbTable[fcbNummer].objID,filePointer,readingBytes,&buffer);  
  //printf("End reading file\n");
  }
  fcbTable[fcbNummer].position+=readingBytes; // 
  *size = readingBytes;
  *data = buffer;
  return readingBytes;
}

IDL4_PUBLISH_FILESVR_READ(file_read_implementation);

IDL4_INLINE CORBA_long file_write_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long length, CORBA_long size, CORBA_char *data, idl4_server_environment *_env)

{
  return ESUPP;
}

IDL4_PUBLISH_FILESVR_WRITE(file_write_implementation);

IDL4_INLINE CORBA_long file_seek_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long pos, idl4_server_environment *_env)

{
  if (!mounted) return ESUPP;
  
  //find fcb
  int objFound = 0;
  int fcbNummer= 0;
  while((objFound==0)&&(fcbNummer<MAXFCB-1)){
    if ((fcbTable[fcbNummer].taskID==_caller)&&(fcbTable[fcbNummer].objID==(int)objID)
	&&(fcbTable[fcbNummer].active==1)){
      objFound=1;
    }
    else
      fcbNummer++;
  }
  if (objFound==0){
    return ENOTOPEN;
  }
  // set pos by found fcb
  else{
    fcbTable[fcbNummer].position=(unsigned int)pos;
    if (fcbTable[fcbNummer].position>fcbTable[fcbNummer].file_length)
      fcbTable[fcbNummer].position=fcbTable[fcbNummer].file_length;
    return ESUCCESS;
  }
}

IDL4_PUBLISH_FILESVR_SEEK(file_seek_implementation);

IDL4_INLINE CORBA_long file_tell_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long *pos, idl4_server_environment *_env)

{
  if (!mounted) return ESUPP;

  //find fcb
  int objFound = 0;
  int fcbNummer= 0;
  while((objFound==0)&&(fcbNummer<MAXFCB-1)){
    if ((fcbTable[fcbNummer].taskID==_caller)&&(fcbTable[fcbNummer].objID==(int)objID)
	&&(fcbTable[fcbNummer].active==1)){
      objFound=1;
    }
    else
      fcbNummer++;
  }
  if (objFound==0){
    return ENOTOPEN;
  }
  // give position back
  else{
    *pos = fcbTable[fcbNummer].position;  
    return ESUCCESS;
  }  
}

IDL4_PUBLISH_FILESVR_TELL(file_tell_implementation);

IDL4_INLINE CORBA_long file_delete_implementation(CORBA_Object _caller, CORBA_long objID, idl4_server_environment *_env)

{
  return ESUPP;
}

IDL4_PUBLISH_FILESVR_DELETE(file_delete_implementation);

IDL4_INLINE CORBA_long filesystem_mount_implementation(CORBA_Object _caller, CORBA_long objID, l4_threadid_t *msvrID, CORBA_long mobjID, idl4_server_environment *_env)

{
  handle_t bdev;  // Handle for Blockdevice
  if (mounted) return EEXISTS;
  
  bdev.svr=*msvrID;
  bdev.obj=mobjID;
  int value = get_superblock(bdev);
  mounted=true;
  return value;
}

IDL4_PUBLISH_FILESVR_MOUNT(filesystem_mount_implementation);

IDL4_INLINE CORBA_long filesystem_unmount_implementation(CORBA_Object _caller, CORBA_long objID, idl4_server_environment *_env)

{
  if (!mounted) return ENOTFOUND;
  mounted=false;
  return ESUCCESS;
}

IDL4_PUBLISH_FILESVR_UNMOUNT(filesystem_unmount_implementation);
