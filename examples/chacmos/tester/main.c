#include <stdlib.h>

#include "chacmos.h"
#include "interfaces/constants_client.h"
#include "interfaces/generic_client.h"
#include "interfaces/task_client.h"
#include "interfaces/file_client.h"
#include "interfaces/directory_client.h"
#include "interfaces/block_client.h"
#include "interfaces/memory_client.h"
#include "interfaces/filesystem_client.h"

//#define TEST_FILESVR
//#define TEST_BLOCKDEVICE

l4_threadid_t myid;

sdword api_task[] = { GENERIC_API, TASK_API };
sdword api_directory[] = { GENERIC_API, FILE_API, DIRECTORY_API };
sdword api_memory[] = { GENERIC_API, MEMORY_API };
sdword api_block[] = { GENERIC_API, BLOCK_API };
sdword api_file[] = { GENERIC_API, FILE_API };
sdword api_filesys[] = { GENERIC_API, FILESYSTEM_API };

void fail(char *why)

{
  printf(" *** %s\n",why);
  enter_kdebug("test aborted");
}  

int main(l4_threadid_t tasksvr, int taskobj)

{
  CORBA_Environment env = idl4_default_environment;
  l4_threadid_t rootsvr, memsvr;
  int rootobj, memobj;
  int x,y,z;
  unsigned int dummyint;
  l4_threadid_t mypagerid, dummyid, t;
  char *buf;

  myid = l4_myself();
  
  printf("Test client started (%X), ",myid.raw);

  dummyid = mypagerid = L4_INVALID_ID;dummyint=0xFFFFFFFF;
  l4_thread_ex_regs(myid, 0xffffffff, 0xffffffff, &dummyid, &mypagerid,
		    &dummyint, &dummyint, &dummyint);

  printf("pager is %X\n\n",mypagerid.raw);

  dummyid = mypagerid = L4_INVALID_ID;
  l4_thread_ex_regs(myid, 0xffffffff, 0xffffffff, &dummyid, &mypagerid,
		    &dummyint, &dummyint, &dummyint);

  printf("========================== chacmOS tester V1.00 ==========================\n");
  printf(" T01 task object is <%Xh,%d>\n",tasksvr.raw,taskobj);
  if (generic_implements(tasksvr,taskobj,sizeof(api_task)/sizeof(int),(sdword*)&api_task,&env)!=ESUCCESS)
    fail("task server does not implement generic or task api");
  
  if (task_get_root(tasksvr,taskobj,&rootsvr,&rootobj,&env)!=ESUCCESS)
    fail("cannot get root from task server");
  printf(" T02 root is <%Xh,%d>\n",rootsvr.raw,rootobj);
  if (generic_implements(rootsvr,rootobj,sizeof(api_directory)/sizeof(int),(sdword*)&api_directory,&env)!=ESUCCESS)
    fail("root does not implement generic or file or directory api");
    
  if (task_get_memory(tasksvr,taskobj,&memsvr,&memobj,&env)!=ESUCCESS)
    fail("cannot get memory object from task server");
  printf(" T03 this task's memory object is <%Xh,%d>\n",memsvr.raw,memobj);
  if (generic_implements(memsvr,memobj,sizeof(api_memory)/sizeof(int),(sdword*)&api_memory,&env)!=ESUCCESS)
    fail("memory server does not implement generic or memory api");
  if (memory_get_pagerid(memsvr,memobj,&x,&env)!=ESUCCESS)
    fail("memory server won't tell me my pager id");
  if (x!=(int)mypagerid.raw)
    fail("my pager id does not match the memory server");

  if (task_get_cmdline(tasksvr,taskobj,&x,&buf,&env)!=ESUCCESS)
    fail("cannot read my command line");
  printf(" T04 my command line is '%s'\n",buf);
  CORBA_free(buf);
  
  if (task_get_threadid(tasksvr,taskobj,&t,&env)!=ESUCCESS)
    fail("task server won't tell me my thread id");
  if (t != myid)
    fail("task server lied about my thread id");

  if (file_open(rootsvr,rootobj,0,&env)!=ESUCCESS)
    fail("cannot open root directory");
  if (file_tell(rootsvr,rootobj,&x,&env)!=ESUCCESS)  
    fail("cannot get file pointer position");
  if (x!=0)
    fail("file pointer is not reset upon open");
  if (file_seek(rootsvr,rootobj,-1,&env)!=ESUCCESS)
    fail("cannot set file pointer to end of file");  
  if (file_tell(rootsvr,rootobj,&x,&env)!=ESUCCESS)  
    fail("cannot get file pointer position (second time)");
  if (file_seek(rootsvr,rootobj,0,&env)!=ESUCCESS)
    fail("cannot set file pointer to beginning of file");  
  printf(" N01 root directory has %d bytes\n",x);
  
  if (x>79) x=79;
  y=file_read(rootsvr,rootobj,x,&z,&buf,&env);
  if (y<=0)
    {
      printf("err code %d\n",y);
      fail("cannot read root directory");
    }  
  if (z!=y)
    fail("return value of open() and data length don't match");
  if (z!=x)
    fail("cannot read all of root directory");
  buf[z]=0;  
  printf(" N02 files contained: (");
  x=0;
  while (z>0) 
    { 
      printf("'%s'",&buf[x]);
      while (buf[x++]) z--;
    }
  printf(")\n");
  if (file_close(rootsvr,rootobj,&env)!=ESUCCESS)
    fail("cannot close root directory\n");
    
  CORBA_free(buf);

  printf("\n");
  
  #ifdef TEST_FILESVR

  handle_t fatfs, dev, hda;

  if (directory_resolve(rootsvr,rootobj,3,"dev",&dev.svr,&dev.obj,&env)!=ESUCCESS)
    fail("cannot find /dev");
  if (generic_implements(dev.svr,dev.obj,sizeof(api_directory)/sizeof(int),(sdword*)&api_directory,&env)!=ESUCCESS)
    fail("/dev does not implement directory or file or generic api");
  if (directory_resolve(dev.svr,dev.obj,3,"hda",&hda.svr,&hda.obj,&env)!=ESUCCESS)
    fail("cannot find /dev/hda");
  if (generic_implements(hda.svr,hda.obj,sizeof(api_block)/sizeof(int),(sdword*)&api_block,&env)!=ESUCCESS)
    fail("/dev/hda does not implement block or generic api");
  printf(" F01 found /dev/hda at <%X,%d>\n",hda.svr.raw,hda.obj);

  if (directory_resolve(dev.svr,dev.obj,5,"fatfs",&fatfs.svr,&fatfs.obj,&env)!=ESUCCESS)
    fail("cannot find /dev/fatfs");
  if (generic_implements(fatfs.svr,fatfs.obj,sizeof(api_filesys)/sizeof(int),(sdword*)&api_filesys,&env)!=ESUCCESS)
    fail("/dev/fatfs does not implement filesystem or generic api");
  printf(" F02 located /dev/fatfs at <%X,%d>\n",hda.svr.raw,hda.obj);

  if (filesystem_mount(fatfs.svr,fatfs.obj,&hda.svr,hda.obj,&env)!=ESUCCESS)
    fail("cannot mount /dev/hda into /dev/fatfs");
  printf(" F03 mounted filesystem\n");

  x=79;
  y=file_read(fatfs.svr,fatfs.obj,x,&z,&buf,&env);
  if (y<=0)
    fail("cannot read root directory of /dev/fatfs");
  if (z>79) z=79;  
  buf[z]=0;  
  printf(" N02 files contained: (");
  x=0;
  while (z>0) 
    { 
      printf("'%s'",&buf[x]);
      while (buf[x++]) z--;
    }
  printf(")\n");
  if (file_close(fatfs.svr,fatfs.obj,&env)!=ESUCCESS)
    fail("cannot close root directory of /dev/fatfs\n");

  CORBA_free(buf);

  printf("\n");
 
  #endif /*TEST_FILESVR*/

  #ifdef TEST_BLOCKDEVICE
  
  int capacity, blksize, size;

  if (directory_resolve(rootsvr,rootobj,3,"dev",&dev.svr,&dev.obj,&env)!=ESUCCESS)
    fail("cannot find /dev");
  if (generic_implements(dev.svr,dev.obj,sizeof(api_directory)/sizeof(int),(sdword*)&api_directory,&env)!=ESUCCESS)
    fail("/dev does not implement directory or file or generic api");
  if (directory_resolve(dev.svr,dev.obj,3,"hda",&hda.svr,&hda.obj,&env)!=ESUCCESS)
    fail("cannot find /dev/hda");
  if (generic_implements(hda.svr,hda.obj,sizeof(api_block)/sizeof(int),(sdword*)&api_block,&env)!=ESUCCESS)
    fail("/dev/hda does not implement block or generic api");
  printf(" B01 found /dev/hda at <%X,%d>\n",hda.svr.raw,hda.obj);
  
  if (block_get_capacity(hda.svr,hda.obj,&capacity,&env)!=ESUCCESS)
    fail("/dev/hda won't tell me its capacity");
  if (block_get_blocksize(hda.svr,hda.obj,&blksize,&env)!=ESUCCESS)
    fail("/dev/hda won't tell me its block size");
  printf(" B02 device reports %d blocks of %d bytes each (%dk)\n",capacity,blksize,(capacity*blksize)/1024);

  if (block_read(hda.svr,hda.obj,1,1,&size,&buf,&env)!=ESUCCESS)
    fail("cannot read block 1 of /dev/hda");
  buf[10]=0;  
  printf(" B03 block 1 reads '%s'\n",buf);
  
  strcpy((char*)&buf[3],"Variatio delectat");
  if (block_write(hda.svr,hda.obj,1,1,21,buf,&env)!=ESUCCESS)
    fail("cannot write block 1 of /dev/hda");
  CORBA_free(buf);
  
  if (block_read(hda.svr,hda.obj,1,1,&size,&buf,&env)!=ESUCCESS)
    fail("cannot reread block 1 of /dev/hda");
  buf[10]=0;  
  printf(" B04 after write, it is '%s'\n",buf);
  CORBA_free(buf);

  #endif /*TEST_BLOCKDEVICE*/

  printf(" X01 test completed successfully\n");

  while (42);  
}
