#include <stdlib.h>

#include "chacmos.h"
#include "booter.h"
#include "interfaces/pager_client.h"
#include "interfaces/pager_server.h"

#ifdef DEBUG
#define dprintf(a...) printf(a)
#else
#define dprintf(a...)
#endif

static l4_threadid_t mypagerid;

IDL4_INLINE void pager_pagefault_implementation(CORBA_Object _caller, const CORBA_long addr, const CORBA_long ip, idl4_fpage_t *p, idl4_server_environment *_env)

{
  dprintf("RPG: Got pagefault at %Xh, ip=%Xh\n", addr, ip);

  if (addr<4096)
    {   
      unsigned dummy;
      dummy = *((volatile int*)&_trampoline); // ensure trampoline is mapped
      idl4_fpage_set_base(p, addr);
      idl4_fpage_set_page(p, l4_fpage((int)&_trampoline,L4_LOG2_PAGESIZE,0,0));
      idl4_fpage_set_permissions(p, IDL4_PERM_READ|IDL4_PERM_WRITE|IDL4_PERM_EXECUTE);
      idl4_fpage_set_mode(p, IDL4_MODE_MAP);
    } else {
             CORBA_Environment env = idl4_default_environment;
             idl4_fpage_t reply;
             
             idl4_set_rcv_window(&env, l4_fpage(0x80000000, L4_LOG2_PAGESIZE,0,0));
             pager_pagefault(mypagerid, 0xFFFFFFFC, 0, &reply, &env);
             if (env._major)
               panic("IPC to sigma0 failed: %d.%X", env._major, CORBA_exception_id(&env));
             dprintf("RPG: Sigma0 sends page %X, base=%X\n", idl4_fpage_get_page(reply).raw, idl4_fpage_get_base(reply));

             idl4_fpage_set_base(p, addr);
             idl4_fpage_set_page(p, l4_fpage(0x80000000, L4_LOG2_PAGESIZE,0,0));
             idl4_fpage_set_permissions(p, IDL4_PERM_READ|IDL4_PERM_WRITE|IDL4_PERM_EXECUTE);
             idl4_fpage_set_mode(p, IDL4_MODE_GRANT);
           }  
           
  dprintf("RPG: Replying fpage=%X, base=%X\n", idl4_fpage_get_page(*p).raw, idl4_fpage_get_base(*p));
}

IDL4_PUBLISH_PAGER_PAGEFAULT(pager_pagefault_implementation);

void *pager_vtable[PAGER_DEFAULT_VTABLE_SIZE] = PAGER_DEFAULT_VTABLE;

void pager_server()

{
  unsigned int msgdope, dummy, fnr, reply, w0, w1, w2;
  l4_threadid_t partner;
  l4_threadid_t dummyid, myid;
  unsigned int dummyint;
  struct {
    unsigned int stack[768];
    l4_fpage_t rcv_window;
    unsigned int size_dope;
    unsigned int send_dope;
    unsigned int message[PAGER_MSGBUF_SIZE];
    idl4_strdope_t str[PAGER_STRBUF_SIZE];
  } buffer;

  // *** find out the threadID of the boss pager (this would be sigma0)
  
  myid = l4_myself();

  dummyid = mypagerid = L4_INVALID_ID;
  l4_thread_ex_regs(myid, 
		    0xffffffff, /* invalid eip */
		    0xffffffff, /* invalid esp */
		    &dummyid, &mypagerid,
		    &dummyint, &dummyint, &dummyint);

  // *** server loop

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
