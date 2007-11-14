#include <stdlib.h>

#include "chacmos.h"
#include "interfaces/namesvr_server.h"
#include "interfaces/task_client.h"
#include "interfaces/directory_client.h"

#define ADVERT_STACK_SIZE 512

handle_t mytaskobj;
int advert_stack[ADVERT_STACK_SIZE];
sdword dirapi[] = { GENERIC_API, FILE_API, DIRECTORY_API };

void advert_thread(void)

{
  CORBA_Environment env = idl4_default_environment;
  l4_threadid_t mySvr = l4_myself();
  handle_t root, dev;
  
  if (task_get_root(mytaskobj.svr,mytaskobj.obj,&root.svr,&root.obj,&env)!=ESUCCESS)
    enter_kdebug("cannot get root");

  if (directory_resolve(root.svr,root.obj,3,"dev",&dev.svr,&dev.obj,&env)!=ESUCCESS)
    enter_kdebug("cannot find /dev");

  mySvr.id.thread = 0;
  if (directory_link(dev.svr,dev.obj,7,"namesvr",&mySvr,0,&env)!=ESUCCESS)
    enter_kdebug("cannot link memory");
  
  while (42);
}

void advertise(l4_threadid_t tasksvr, int taskobj)

{
  l4_threadid_t mypagerid, tid, dummyid, myid = l4_myself();
  dword_t dummyint;
  
  mytaskobj.svr=tasksvr;
  mytaskobj.obj=taskobj;

  dummyid = mypagerid = L4_INVALID_ID;
  dummyint = 0xFFFFFFFF;
  l4_thread_ex_regs(myid, 0xffffffff, 0xffffffff, &dummyid, &mypagerid,
		    &dummyint, &dummyint, &dummyint);

  myid.id.thread = 4; 
  tid = mypagerid;
  dummyid = L4_NIL_ID;

  l4_thread_ex_regs(myid, (int) advert_thread, 
		    (dword_t) &advert_stack[ADVERT_STACK_SIZE-1],
		    &dummyid, &tid, &dummyint,&dummyint,&dummyint);
}

