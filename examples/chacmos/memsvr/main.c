#include <stdlib.h>

#include "interfaces/memsvr_server.h"
#include "interfaces/pager_client.h"
#include "interfaces/pager_server.h"
#include "memsvr.h"

#define L4_IPC_SHORT_FPAGE 2

address_space_t space[MAXTASKS];
int *mtab = (int*)MTAB_BASE;
dword_t pager_stack[PAGER_STACK_SIZE];

int removepage(int nr, int fromspace)

{
  int pos=space[fromspace].root;

  if (pos==-1) 
    return 0;
    
  if (pos==nr)
    {
      space[fromspace].root=mtab[pos];
      mtab[pos]=-1;
    } else {
             while ((mtab[pos]!=nr) && (mtab[pos]>=0))
               pos=mtab[pos];
               
             if (mtab[pos]<0) 
               return 0;  
    
             mtab[pos]=mtab[mtab[pos]];
             mtab[nr]=-1;
           }

  space[fromspace].pages--;
  
  return 1;             
}

int addpage(int nr, int tospace)

{
  if (mtab[nr]>=0)
    return 0;

  mtab[nr]=space[tospace].root;
  space[tospace].root=nr;
  space[tospace].pages++;
  
  return 1;
}

IDL4_INLINE void pager_pagefault_implementation(CORBA_Object _caller, const CORBA_long addr, const CORBA_long ip, idl4_fpage_t *p, idl4_server_environment *_env)

{
  #ifdef DEBUG
  printf("Got a pagefault from %X, addr %X, IP=%X\n",_caller.raw, addr, ip);
  #endif
    
  int clientnr = 2;
  while ((clientnr<MAXTASKS) && (space[clientnr].thread_id!=_caller))
    clientnr++;

  idl4_fpage_set_base(p, addr & 0xFFFFF000);
  idl4_fpage_set_mode(p, IDL4_MODE_MAP);
  
  if (clientnr<MAXTASKS)
    {  
      if (addr!=1)
        {
          if ((addr>=0xB8000) && (addr<=0xC0000))
            {
              idl4_fpage_set_page(p, l4_fpage(addr&0xFFFFF000 + POOL_BASE,L4_LOG2_PAGESIZE,0,0));
              idl4_fpage_set_permissions(p, IDL4_PERM_READ|IDL4_PERM_WRITE|IDL4_PERM_EXECUTE);
            } else {
                     if ((space[FREELIST].pages>0) && (space[clientnr].pages<space[clientnr].maxpages))
                       {
                         int page=space[FREELIST].root;
                         removepage(page,FREELIST);
                         addpage(page,clientnr);
                         idl4_fpage_set_page(p, l4_fpage(POOL_BASE+4096*page, L4_LOG2_PAGESIZE, 0, 0));
                         idl4_fpage_set_permissions(p, IDL4_PERM_READ|IDL4_PERM_WRITE|IDL4_PERM_EXECUTE);
                       } else idl4_set_no_response(_env);
                   }
        } else {
                 idl4_fpage_set_page(p, l4_fpage(INFO_BASE, L4_LOG2_PAGESIZE, 0, 0));
                 idl4_fpage_set_permissions(p, IDL4_PERM_READ);
               } 
    } else { 
             idl4_set_no_response(_env);
             enter_kdebug("not allowed by memsvr"); 
           }
}

IDL4_PUBLISH_PAGER_PAGEFAULT(pager_pagefault_implementation);

void *pager_vtable[PAGER_DEFAULT_VTABLE_SIZE] = PAGER_DEFAULT_VTABLE;

void pager_server()

{
  unsigned int msgdope, dummy, fnr, reply, w0, w1, w2;
  l4_threadid_t partner;
  struct {
    unsigned int stack[768];
    l4_fpage_t rcv_window;
    unsigned int size_dope;
    unsigned int send_dope;
    unsigned int message[PAGER_MSGBUF_SIZE];
    idl4_strdope_t str[PAGER_STRBUF_SIZE];
  } buffer;

  while (1)
    {
      buffer.size_dope = PAGER_RCV_DOPE;
      buffer.rcv_window = idl4_nilpage;
      partner = idl4_nilthread;
      reply = idl4_nil;
      w0 = w1 = w2 = 0;

      while (1)
        {
          idl4_reply_and_wait(reply, buffer, partner, msgdope, fnr, w0, w1, w2, dummy);

          if (msgdope & 0xF0)
            break;

          idl4_process_request(pager_vtable, fnr & PAGER_FID_MASK, reply, buffer, partner, w0, w1, w2, dummy);
        }

      enter_kdebug("message error");
    }
}

void pager_discard()

{
}

int page_reserved(int page_nr, kernel_info_page_t *infopage)

{
  unsigned int addr = page_nr<<12;

  if (((infopage->reserved0.low<=addr) && (infopage->reserved0.high>addr)) ||
      ((infopage->reserved1.low<=addr) && (infopage->reserved1.high>addr)) ||
      ((infopage->dedicated0.low<=addr) && (infopage->dedicated0.high>addr)) ||
      ((infopage->dedicated1.low<=addr) && (infopage->dedicated1.high>addr)) ||
      ((infopage->dedicated2.low<=addr) && (infopage->dedicated2.high>addr)) ||
      ((infopage->dedicated3.low<=addr) && (infopage->dedicated3.high>addr)) ||
      ((infopage->dedicated4.low<=addr) && (infopage->dedicated4.high>addr)))
    return 1;
      
  return 0;
}
  
int main(l4_threadid_t tasksvr, int taskobj)                 

{
  dword_t dw0, dw1, dw2;
  l4_msgdope_t result;
  unsigned int dummyint, mtabsize, mapbase;
  signed int i,j,pagecnt;
  l4_threadid_t mypagerid, dummyid, myid, pager_threadid, tid;
  kernel_info_page_t *infopage;

  // *** find out the threadID of the boss pager (this would be sigma0)
  
  myid = l4_myself();

  dummyid = mypagerid = L4_INVALID_ID;
  l4_thread_ex_regs(myid, 0xffffffff, 0xffffffff, &dummyid, &mypagerid,
		    &dummyint, &dummyint, &dummyint);

  #ifdef DEBUG
  printf("Memory server (%X) starting, pager is %X...\n",myid.raw,mypagerid.raw);
  #endif
  pagecnt=0;

  // *** map kernel info page
  
  l4_ipc_call(mypagerid,0,1,1,0,(void*)(INFO_BASE+0x32),&dw0,&dw1,&dw2,L4_IPC_NEVER,&result);  
  infopage=(kernel_info_page_t*)INFO_BASE;
  if (infopage->ln_magic!=0x4be6344c) 
    enter_kdebug("bad kernel info page");
    
  #ifdef DEBUG
  printf("%dM reported by kernel, snatching pages...\n",(infopage->main.high/1048576));
  #endif
  
  pagecnt=infopage->main.high/4096;
  mtabsize=(pagecnt+1023)/1024;

  // *** init all address spaces
  
  for (int i=0;i<MAXTASKS;i++)
    {
      space[i].root=-1;
      space[i].pages=0;
      space[i].maxpages=0x7FFFFFFF;
      space[i].thread_id.raw=0xFFFFFFFF;
      space[i].boss_id.raw=0xFFFFFFFF;
    }  

  // *** snatch all available pages and build freelist

  mapbase=MTAB_BASE;mtab=(int*)MTAB_BASE;
  for (i=(pagecnt-1);i>=0;i--)
    {
      if (!page_reserved(i,infopage))
        l4_ipc_call(mypagerid,0,i*4096,0,0,(void*)(mapbase+0x32),&dw0,&dw1,&dw2,L4_IPC_NEVER,&result);
        else dw0=0;
        
      if (mtabsize)
        {
          // we are still allocating the mapping database. this must be
          // a contiguous chunk of memory, so we use the first pages we
          // get and map them at MTAB_BASE
        
          if (dw0) 
            { 
              mapbase+=4096;mtabsize--;
              if (!mtabsize) 
                {
                  mapbase=POOL_BASE;
                  for (j=0;j<pagecnt;j++)
                    mtab[j]=-1;
                }    
            }
        } else {  
                 // we are building the free page pool
        
                 if (dw0) 
                   addpage(i,FREELIST);
                 mapbase+=4096;
               }    
    }  
    
  #ifdef DEBUG
  printf("%d pages (%dk) received\n",space[FREELIST].pages,
         (space[FREELIST].pages*4));
  #endif

  // *** reserve video frame buffer for physical mapping (on request)
  
  for (i=(0xB8000/4096);i<(0xC0000/4096);i++)
    if (removepage(i,FREELIST))
      addpage(i,PHYSICAL);

  #ifdef DEBUG
  printf("Memory reserved for physical mapping: %d pages\n",space[PHYSICAL].pages);
  #endif

  // *** start pager and advert thread

  pager_threadid = myid;
  pager_threadid.id.thread = 1; 
  tid = mypagerid;
  dummyid = L4_NIL_ID;

  l4_thread_ex_regs(pager_threadid, (int) pager_server, 
		    (dword_t) &pager_stack[PAGER_STACK_SIZE-1],
		    &dummyid, &tid, &dummyint,&dummyint,&dummyint);

  advertise(tasksvr, taskobj);
                    
  memsvr_server();
}
