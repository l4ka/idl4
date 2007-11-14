#include "stdlib.h"
#include "interfaces/example_server.h"

int main(l4_threadid_t tasksvr, int taskobj)                 

{
  printf("Example server is running, entering server loop...\n\n");
  example_server();
}
