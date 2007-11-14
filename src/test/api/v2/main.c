#define IDL4_INTERNAL
#include <idl4/test.h>

#define dprintf(a...)

char server_stack[8192], pager_stack[4096], control_stack[4096];
l4_threadid_t sigma0, pager, control, server;

void pager_thread(void);

void idl4_thread_init(l4_threadid_t __attribute__ ((unused)) thread, unsigned int eip)

{
  unsigned dw0 = 0xc01ad05e, dw1;
  l4_msgdope_t result;
  
  l4_i386_ipc_call(control, (void*)0, dw0, eip, 
                   (void*)0, &dw0, &dw1, L4_IPC_NEVER, &result);
              
  if (dw0 != 0xaffeaffe)
    panic("Malformed reply in idl4_thread_init");
}

void idl4_start_thread(l4_threadid_t target, l4_threadid_t pager, char *esp, void (*eip)())

{
  l4_threadid_t nil = { lh: { 0xFFFFFFFF, 0xFFFFFFFF } };
  l4_umword_t dummy = 0xFFFFFFFF;
  l4_thread_ex_regs(target, (unsigned)eip, (unsigned)esp, (l4_threadid_t*)&nil, 
                    &pager, &dummy, &dummy, &dummy);
} 

void control_thread(void)

{
  unsigned dw0, dw1;
  l4_threadid_t partner;
  l4_msgdope_t result;
  
  l4_i386_ipc_wait(&partner, (void*)L4_IPC_OPEN_IPC, &dw0, &dw1,
                   L4_IPC_NEVER, &result);
  
  do {
       if (dw0 != 0xc01ad05e)
         panic("Malformed request in control_thread (dw0=%Xh)");
         
       dprintf("Starting thread at %Xh\n", dw1);
       
       idl4_start_thread(server, pager, &server_stack[sizeof(server_stack)-4], (void(*)())dw1);
       dw0 = 0xaffeaffe;

       l4_i386_ipc_reply_and_wait(partner, (void*)0, dw0, dw1,
                                  &partner, (void*)L4_IPC_OPEN_IPC, &dw0, &dw1,
                                  L4_IPC_NEVER, &result);
     } while (1);
}

l4_threadid_t idl4_get_pager()

{
  l4_threadid_t preempter = L4_INVALID_ID, pager = L4_INVALID_ID;
  l4_umword_t dummy;
  
  l4_thread_ex_regs(l4_myself(), 0xffffffff, 0xffffffff, &preempter, 
                    &pager, &dummy, &dummy, &dummy);
                    
  return pager;
}

int main(void)

{
  l4_taskid_t newtask;
  
  pager = l4_myself();
  pager.id.lthread++;
  sigma0 = idl4_get_pager();
  
  dprintf("Hello, I am %X.%d\n", l4_myself().id.task, l4_myself().id.lthread);
  dprintf("Root pager is %X.%d, now starting %X.%d with this pager\n", 
          sigma0.id.task, sigma0.id.lthread, pager.id.task, pager.id.lthread);
  
  idl4_start_thread(pager, sigma0, &pager_stack[sizeof(pager_stack)-4], pager_thread);

  control = l4_myself();
  control.id.task++;
  control.id.lthread = 0;
  control.id.chief = l4_myself().id.task;
  
  dprintf("Creating task %X.%d with %X.%d as pager\n", control.id.task, control.id.lthread, pager.id.task, pager.id.lthread);

  newtask = l4_task_new(control, 255, (unsigned)&control_stack[sizeof(control_stack)-4], (unsigned)control_thread, pager); 
  dprintf("New task is: %X.%d\n", newtask.id.task, newtask.id.lthread);
  if (newtask.id.task==0)
    panic("Cannot create task - clans&chiefs?");

  server = control;
  server.id.lthread++;
  
  dprintf("The server thread will be %X.%d\n", server.id.task, server.id.lthread);

  test(server);
  
  return 0;
}

