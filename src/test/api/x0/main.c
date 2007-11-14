#define IDL4_INTERNAL
#include <idl4/test.h>

#define dprintf(a...)

char server_stack[16384], pager_stack[4096], control_stack[4096];
void pager_thread(void);

l4_threadid_t sigma0 = { raw: 0x4020001 };
l4_threadid_t pager = { raw: 0x4040401 };
l4_threadid_t control = { raw: 0x4050001 };
l4_threadid_t server = { raw: 0x4050401 };

void idl4_thread_init(l4_threadid_t __attribute__ ((unused)) thread, unsigned int eip)

{
  unsigned dw0 = 0xc01ad05e, dw1, dw2;
  l4_msgdope_t result;
  
  l4_ipc_call(control, (void*)0, dw0, eip, 0, 
              (void*)0, &dw0, &dw1, &dw2, L4_IPC_NEVER, &result);
              
  if (dw0 != 0xaffeaffe)
    panic("Malformed reply in idl4_thread_init");
}

void idl4_start_thread(l4_threadid_t target, l4_threadid_t pager, char *esp, void (*eip)())

{
  dword_t dummy = 0xFFFFFFFF;
  l4_thread_ex_regs(target, (unsigned)eip, (unsigned)esp, (l4_threadid_t*)&dummy, 
                    &pager, &dummy, &dummy, &dummy);
} 

void control_thread(void)

{
  unsigned dw0, dw1, dw2;
  l4_threadid_t partner;
  l4_msgdope_t result;
  
  l4_ipc_wait(&partner, (void*)L4_IPC_OPEN_IPC, &dw0, &dw1, &dw2,
              L4_IPC_NEVER, &result);
  
  do {
       if (dw0 != 0xc01ad05e)
         panic("Malformed request in control_thread (dw0=%Xh)");
         
       dprintf("Starting thread at %Xh\n", dw1);
       
       idl4_start_thread(server, pager, &server_stack[sizeof(server_stack)-4], (void(*)())dw1);
       dw0 = 0xaffeaffe;

       l4_ipc_reply_and_wait(partner, (void*)0, dw0, dw1, dw2,
                             &partner, (void*)L4_IPC_OPEN_IPC, &dw0, &dw1, &dw2,
                             L4_IPC_NEVER, &result);
     } while (1);
}

void idl4_set_pager(l4_threadid_t pager)

{
  l4_threadid_t preempter = L4_INVALID_ID;
  unsigned dummy = 0xFFFFFFFF;
  
  l4_thread_ex_regs(l4_myself(), 0xFFFFFFFF, 0xFFFFFFFF, &preempter,
                    &pager, &dummy, &dummy, &dummy);
}

int main(void)

{
  l4_threadid_t pager = l4_myself();
  pager.id.thread++;
  idl4_start_thread(pager, sigma0, &pager_stack[sizeof(pager_stack)-4], pager_thread);
  idl4_set_pager(pager);

  l4_task_new(control, 255, (unsigned)&control_stack[sizeof(control_stack)-4], (unsigned)control_thread, pager);

  test(server);
  
  return 0;
}

