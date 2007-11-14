#ifndef __namesvr_h
#define __namesvr_h

//#define DEBUG

#define MAXDIRS   10
#define MAXNAMES  100
#define MAXLENGTH 50
#define MAXDATA   1000
#define MAXFILES  20

typedef struct {
                 int active;
                 int first_child;
               } directory_t;  

typedef struct {
                 int active;
                 int next_peer;
                 char ascii[MAXLENGTH];
                 l4_threadid_t svrID;
                 int objID;
               } name_t;  
               
typedef struct {
                 int active;
                 l4_threadid_t threadID;
                 int objID;
                 unsigned int pos, size;
                 char data[MAXDATA];
               } file_t;  

extern name_t nameslot[MAXNAMES];
extern directory_t directory[MAXDIRS];
extern file_t file[MAXFILES];
extern l4_threadid_t mytaskid;

void advertise(l4_threadid_t tasksvr, int taskobj);

#endif
