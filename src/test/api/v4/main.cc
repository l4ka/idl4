#define IDL4_INTERNAL
#include <idl4/test.h>
#include <l4/kip.h>

#define dprintf(a...)

#if defined(CONFIG_ARCH_IA64)
#define GET_IP(x) (*(L4_Word_t*)(x))
#else
#define GET_IP(x) ((L4_Word_t)(x))
#endif

L4_Word_t client_stack[2048], server_stack[2048], pager_stack[2048];
L4_ThreadId_t master, pager, client, server, sigma0;

extern int __textstart;

#if defined(CONFIG_ARCH_IA64) || defined(CONFIG_ARCH_ALPHA)
extern L4_Word_t _exreg_target;
#endif

extern "C" 

{ 
  void test(L4_ThreadId_t server); 
  void idl4_thread_init(L4_ThreadId_t thread, L4_Word_t eip);
}

void idl4_thread_init(L4_ThreadId_t thread, L4_Word_t eip)

{
  L4_Word_t old_control, old_sp, old_ip, old_flags, old_UserDefinedHandle;
  L4_ThreadId_t old_pager;

#if defined(CONFIG_ARCH_IA64) || defined(CONFIG_ARCH_ALPHA)
  server_stack[2047] = GET_IP(eip);
  server_stack[2046] = (L4_Word_t)&server_stack[2046];
  
  L4_ExchangeRegisters(thread, 0x9E /* p--isSR- */, 
      (long)&server_stack[2047], (L4_Word_t)&_exreg_target, 0, 0, pager,
      &old_control, &old_sp, &old_ip, &old_flags, &old_UserDefinedHandle,
      &old_pager);
#else
  L4_ExchangeRegisters(thread, 0x9E /* p--isSR- */, 
      (long)&server_stack[1023], eip, 0, 0, pager,
      &old_control, &old_sp, &old_ip, &old_flags, &old_UserDefinedHandle,
      &old_pager);
#endif
} 

void client_thread(void)

{
  test(server);
  enter_kdebug("done");
}

#if defined(CONFIG_ARCH_ALPHA)
#define PAGE_BITS 	13
#else
#define PAGE_BITS 	12
#endif

#define PAGE_SIZE	(1 << PAGE_BITS)
#define PAGE_MASK	(~(PAGE_SIZE-1))

static void send_startup_ipc(L4_ThreadId_t tid, L4_Word_t ip, L4_Word_t sp)

{
  dprintf("sending startup message to %x, (ip=%x, sp=%x)\n", tid.raw, ip, sp);
  L4_Msg_t msg;
  L4_Clear(&msg);
  L4_Append(&msg, ip);
  L4_Append(&msg, sp);
  L4_MsgLoad(&msg);
  L4_Send(tid);
}

#define MAX_RESOURCES   32

void pager_thread(void)

{
  int resource_no = 0;
  L4_ThreadId_t tid;
  L4_MsgTag_t tag;
  L4_Msg_t msg;

  dprintf("pager: entering loop\n");
  while (1)
    {
      tag = L4_Wait(&tid);

      while(1)
	{
	  if (tid == master)
	    {
	      dprintf("pager: sending startup messages\n");
#if defined(CONFIG_ARCH_IA64) || defined(CONFIG_ARCH_ALPHA)
              client_stack[2047] = GET_IP(client_thread);
              client_stack[2046] = (L4_Word_t)&client_stack[2046];
	      send_startup_ipc(client, (L4_Word_t)&_exreg_target, (L4_Word_t)&client_stack[2047]);
#else
	      send_startup_ipc(client, (L4_Word_t)client_thread, (L4_Word_t)client_stack + sizeof(client_stack));
#endif
              L4_ThreadSwitch(server);
	      break;
	    }
	    
	  // pagefault IPC
	  if (L4_UntypedWords(tag) != 2 || L4_TypedWords(tag) != 0 || !L4_IpcSucceeded(tag))
	    {
	      printf("pager: malformed pagefault IPC from %lx (tag=%lx)\n", tid.raw, tag.raw);
	      enter_kdebug("malformed pf");
              break;
	    }
            
	  L4_MsgStore(tag, &msg);
	  dprintf("pager: got page fault message from %x, addr=%x, ip=%x\n", tid.raw, L4_Get(&msg, 0), L4_Get(&msg, 1));
            
          L4_Word_t rsrc_addr = L4_Get(&msg, 0);
          if (rsrc_addr>=0x40000000)
            {
              dprintf("pager: resource fault at %x\n", rsrc_addr);
              if (resource_no>=MAX_RESOURCES)
                enter_kdebug("no resources left");
              rsrc_addr = 0xC00000 + (resource_no++)*4096;
              *((char*)rsrc_addr)=0;
            } else dprintf("pager: ordinary fault at %x\n", rsrc_addr);

	  L4_Clear(&msg);
	  L4_Append(&msg, L4_MapItem(L4_FpageLog2(rsrc_addr, PAGE_BITS) + L4_FullyAccessible, 
	  L4_Get(&msg, 0)));
	    
          dprintf("pager: sending reply\n");
	  L4_MsgLoad(&msg);
	  tag = L4_ReplyWait(tid, &tid);
	}
    }
}


int main(void)

{
  L4_Word_t control;
  L4_Fpage_t kip_area, utcb_area;
  void *utcb0, *utcb1;

  dprintf("master: starting test suite\n");

  sigma0 = L4_Pager();
  client = server = master = pager = L4_Myself();
  pager.global.X.thread_no++;
  client.global.X.thread_no+=2;
  server.global.X.thread_no+=3;
  
  printf("master: sigma0 is %lx (%lx.%lx)\n", sigma0.raw, sigma0.global.X.thread_no, sigma0.global.X.version);
  printf("master: master is %lx (%lx.%lx)\n", master.raw, master.global.X.thread_no, master.global.X.version);
  printf("master: pager  is %lx (%lx.%lx)\n", pager.raw, pager.global.X.thread_no, pager.global.X.version);
  printf("master: client is %lx (%lx.%lx)\n", client.raw, client.global.X.thread_no, client.global.X.version);
  printf("master: server is %lx (%lx.%lx)\n", server.raw, server.global.X.thread_no, server.global.X.version);

  L4_KernelInterfacePage_t * kip = (L4_KernelInterfacePage_t*) L4_KernelInterface();
  utcb0 = (void*)(L4_MyLocalId().raw & ~((1<<kip->UtcbAreaInfo.X.s)-1));
  utcb1 = (void*)((L4_Word_t)utcb0 + (1<<kip->UtcbAreaInfo.X.s));
  dprintf("master: KIP is at %p, UTCB are at %p, %p\n", kip, utcb0, utcb1);

  /* put the kip at the same location in all AS to make sure we
   * can reuse the syscall jump table */
  kip_area = L4_FpageLog2((L4_Word_t)kip, kip->KipAreaInfo.X.s);

  /* we need a maximum of two threads per task */
  utcb_area = L4_FpageLog2(0x80000000, kip->UtcbAreaInfo.X.s + 1);

  // touch the memory to make sure we never get pagefaults
  extern L4_Word_t _end;
  dprintf("master: touching pages from %x to %x\n", (L4_Word_t)&__textstart, (L4_Word_t)&_end);
  for (L4_Word_t *x = (L4_Word_t*)&__textstart; x < &_end; x+=4)
    {
      L4_Word_t q;
      q = *(volatile L4_Word_t*)x;
    }

  /* Create pager */
  dprintf("master: creating pager\n");
  L4_ThreadControl(pager, master, master, sigma0, utcb1);
  /* due to Pistachio problems with thread_startup */
  L4_ThreadSwitch(pager);
  L4_AbortReceive_and_stop(pager);
  
#if defined(CONFIG_ARCH_IA64) || defined(CONFIG_ARCH_ALPHA)
  pager_stack[2047] = GET_IP(pager_thread);
  pager_stack[2046] = (L4_Word_t)&pager_stack[2046];
  L4_Start(pager, (L4_Word_t)&pager_stack[2047], (L4_Word_t)&_exreg_target);
#else
  L4_Start(pager, (L4_Word_t)pager_stack + sizeof(pager_stack), (L4_Word_t)pager_thread);
#endif      

  dprintf("master: entering loop\n");
  for (;;)
    {
      L4_ThreadControl(client, client, master, L4_nilthread, (void*)0x80000000);
      L4_ThreadControl(server, client, master, L4_nilthread, (void*)(L4_Word_t)(0x80000000+(1<<kip->UtcbAreaInfo.X.s)));
      L4_SpaceControl(client, 0, kip_area, utcb_area, L4_nilthread, &control);
      L4_ThreadControl(client, client, master, pager, (void*)-1);
      L4_ThreadSwitch(client);
      L4_ThreadControl(server, client, master, pager, (void*)-1);
      L4_ThreadSwitch(server);
      
      L4_LoadMR(0, 0); // send empty message to notify pager to startup both threads

      L4_Send(pager);

      L4_MsgTag_t tag = L4_Receive(client);
      enter_kdebug("woken up - check");

      // kill both threads
      L4_ThreadControl(client, L4_nilthread, L4_nilthread, L4_nilthread, (void*)-1);
      L4_ThreadControl(server, L4_nilthread, L4_nilthread, L4_nilthread, (void*)-1);
    }

  for (;;)
    enter_kdebug("EOW");
}
