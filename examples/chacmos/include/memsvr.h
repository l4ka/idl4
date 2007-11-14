#ifndef __memsvr_h
#define __memsvr_h

//#define DEBUG

#define MAXTASKS 10

#define POOL_BASE 0x80000000
#define MTAB_BASE 0x70000000
#define INFO_BASE 0x7FFFF000

#define FREELIST 0
#define PHYSICAL 1

#define PAGER_STACK_SIZE 1024

typedef struct {
                 unsigned int low;
                 unsigned int high;
               } mem_region_t;  

typedef struct {
                 unsigned int ln_magic;
                 unsigned int version;
                 unsigned int empty1[22];
                 mem_region_t main,reserved0,reserved1,dedicated0;
                 mem_region_t dedicated1,dedicated2,dedicated3,dedicated4;
                 unsigned int clock_low;
                 unsigned int clock_hgh;
                 unsigned int empty2[2];
                 unsigned int clockfreq;
                 unsigned int busfreq;
                 unsigned int empty3[2];
               } kernel_info_page_t;  
               
typedef struct {
                 int root;
                 int pages;
                 int maxpages;
                 l4_threadid_t thread_id;
                 l4_threadid_t boss_id;
               } address_space_t;                 

extern address_space_t space[MAXTASKS];
extern int *mtab;

void advertise(l4_threadid_t tasksvr, int taskobj);

#endif
