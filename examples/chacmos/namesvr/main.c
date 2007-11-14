#include <stdlib.h>

#include "interfaces/namesvr_server.h"
#include "namesvr.h"

name_t nameslot[MAXNAMES];
directory_t directory[MAXDIRS];
file_t file[MAXFILES];
l4_threadid_t mytaskid;

int main(l4_threadid_t tasksvr, int taskobj)     

{
  int i;

  mytaskid = l4_myself();

  for (i=0;i<MAXDIRS;i++) 
    directory[i].active=0;
  for (i=0;i<MAXNAMES;i++)
    nameslot[i].active=0;  
  for (i=0;i<MAXFILES;i++)
    file[i].active=0;  

  advertise(tasksvr, taskobj);

  namesvr_server();
}
