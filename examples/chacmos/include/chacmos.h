#ifndef CHACMOS_MAIN
#define CHACMOS_MAIN

#define MAXBLOCKS 10
#define MAXTASKS 10

#include <l4/l4.h>

typedef int sdword;

typedef struct {
  l4_threadid_t svr;
  int obj;
} handle_t;

#endif
