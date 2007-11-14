#define IDL4_INTERNAL
#include <idl4/test.h>

#define dprintf(a...)
 
void pager_thread(void) 

{
  int r;
  l4_threadid_t src;
  l4_umword_t dw0, dw1;
  l4_msgdope_t result;
  unsigned int dummyint;
  l4_threadid_t mypagerid, dummyid, myid;

  dprintf("Pager starting up (%X.%d)\n", l4_myself().id.task, l4_myself().id.lthread);

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
      r = l4_i386_ipc_wait(&src, (void*)L4_IPC_OPEN_IPC, &dw0, &dw1,
                           L4_IPC_NEVER, &result);
      while (1) 
        {
          if (r != 0) 
            panic("error on pagefault IPC");

          dprintf("Handling pagefault from %X.%X, dw0=%xh\n", src.lh.high, src.lh.low, dw0);
  
          l4_i386_ipc_call(mypagerid,L4_IPC_SHORT_MSG,dw0,dw1,(void*)((L4_WHOLE_ADDRESS_SPACE<<2) + (int)L4_IPC_SHORT_FPAGE),
                           &dw0,&dw1,L4_IPC_NEVER,&result);
      
          dprintf("Sigma0 replies: %xh, %xh (dope %xh)\n", dw0, dw1, result.msgdope);

          // *** apply the mapping and wait for next fault

          r = l4_i386_ipc_reply_and_wait(src, L4_IPC_SHORT_FPAGE, dw0, dw1,
                                         &src, (void*)L4_IPC_OPEN_IPC, &dw0, &dw1,
                                         L4_IPC_NEVER, &result);
        }                            
    }
}
