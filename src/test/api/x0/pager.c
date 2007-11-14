#define IDL4_INTERNAL
#include <idl4/test.h>

#define dprintf(a...)
 
#define L4_IPC_SHORT_MSG 	((void*)0)
#define L4_IPC_SHORT_FPAGE 	((void*)2)

extern unsigned nopf_start, nopf_end;
extern int __textstart;

void pager_thread(void) 

{
  int r;
  l4_threadid_t src;
  dword_t dw0, dw1, dw2;
  l4_msgdope_t result;
  unsigned int dummyint;
  l4_threadid_t mypagerid, dummyid, myid;

  dprintf("Pager starting up (%X)\n", l4_myself().raw);
  dprintf("Receive area is %Xh..%Xh\n", nopf_start, nopf_end);

  // *** find out the threadID of the boss pager (this would be sigma0)
  
  myid = l4_myself();

  dummyid = mypagerid = L4_INVALID_ID;
  l4_thread_ex_regs(myid, 
		    0xffffffff, /* invalid eip */
		    0xffffffff, /* invalid esp */
		    &dummyid, &mypagerid,
		    &dummyint, &dummyint, &dummyint);

  // *** wait for the first pagefault IPC to occur

  while (1) 
    {
      r = l4_ipc_wait(&src, (void*)L4_IPC_OPEN_IPC, &dw0, &dw1, &dw2,
    		      L4_IPC_NEVER, &result);
      while (1) 
        {
          if (r != 0) 
            panic("error on pagefault IPC");

          dprintf("Handling pagefault from %xh, dw0=%xh\n", src.raw, dw0);
  
          if (dw0>=nopf_start && dw0<=nopf_end)
            panic("Pagefault in forbidden area at %Xh (mapping failed?)", dw0);
            
          if (dw0<(unsigned)&__textstart)
            panic("Pagefault at %Xh (text segment starts at %Xh; source=%Xh, ip=%Xh)", dw0, (unsigned)&__textstart, src.raw, dw1);
            
          l4_ipc_call(mypagerid,L4_IPC_SHORT_MSG,dw0,dw1,dw2,(void*)((L4_WHOLE_ADDRESS_SPACE<<2) + (int)L4_IPC_SHORT_FPAGE),
                      &dw0,&dw1,&dw2,L4_IPC_NEVER,&result);
      
          dprintf("Sigma0 replies: %xh, %xh, %xh (dope %xh)\n", dw0, dw1, dw2, result.raw);

          // *** apply the mapping and wait for next fault

          r = l4_ipc_reply_and_wait(src, L4_IPC_SHORT_FPAGE, dw0, dw1, dw2,
                                    &src, (void*)L4_IPC_OPEN_IPC, &dw0, &dw1, &dw2,
                                    L4_IPC_NEVER, &result);
        }                            
    }
}
