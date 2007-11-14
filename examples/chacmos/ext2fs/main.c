#include <stdlib.h>

#include "chacmos.h"
#include "interfaces/filesvr_server.h"
#include "interfaces/directory_client.h"
#include "interfaces/task_client.h"
#include "filesvr.h"

char myname[10];

int main(l4_threadid_t tasksvr, int taskobj)     

{
  CORBA_Environment env = idl4_default_environment;
  l4_threadid_t myself=l4_myself();
  int serial=0;
  
  //initialize fcbTable, set fcbs inactive
  for (int i=0; i<MAXFCB; i++){
    fcbTable[i].active=0;
    fcbTable[i].file_length=42;
  }

  //link to name server
  // find out who is name server?
  handle_t root_name_server;
  sdword result = task_get_root(tasksvr, taskobj, &root_name_server.svr, &root_name_server.obj, &env);
  if (result != ESUCCESS) enter_kdebug ("get root failed");

  // resolve dev
  handle_t dev_name_server;
  result = directory_resolve(root_name_server.svr, root_name_server.obj, 3, "dev", 
			     &dev_name_server.svr, &dev_name_server.obj, &env);
  if (result != ESUCCESS) enter_kdebug ("resolve dev failed");

  // dev.link(fatfs)
  do {
       if (!(serial++)) strcpy(myname,"ext2fs"); else sprintf(myname,"ext2fs.%d",serial);
       result = directory_link(dev_name_server.svr, dev_name_server.obj, strlen(myname), myname, &myself, 0, &env);
     } while (result==EEXISTS);  
  if (result != ESUCCESS) enter_kdebug ("resolve dev failed");
  filesvr_server();
}
