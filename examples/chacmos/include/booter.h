#ifndef __tasksvr_h
#define __tasksvr_h

//#define DEBUG

#include <l4/l4.h>

#define TRAMPOLINE_RECEIVE   0
#define TRAMPOLINE_ZERO_ZONE 1
#define TRAMPOLINE_NEW_PAGER 2
#define TRAMPOLINE_LAUNCH    3

#define MAXTASKS  10
#define MAXBLOCKS 10
#define MAXLENGTH 50

#define THREAD_STACK_SIZE 1024

typedef struct {
                 int active;
                 handle_t root, memory;
                 char cmdline[MAXLENGTH];
                 l4_threadid_t task_id;
                 l4_threadid_t owner;
               } task_t;  

typedef struct {
                 int active;
                 int phys_addr;
                 int capacity;
                 int blocksize;
               } block_t;  

void pager_server(void);
void server(void);
int elf_decode(int objID, handle_t file);
void strip(int objID);
extern int _trampoline,_m1,_m2;

extern task_t task[MAXTASKS];
extern block_t block[MAXBLOCKS];
extern handle_t root;
extern l4_threadid_t nexttask, memsvr, auxpager, sigma0;

extern int _start, _end;

#endif
