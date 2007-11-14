#include <stdlib.h>

#include "chacmos.h"
#include "elf.h"
#include "interfaces/generic_client.h"
#include "interfaces/file_client.h"
#include "interfaces/memory_client.h"
#include "interfaces/booter_server.h"
#include "booter.h"

sdword api_file[] = { GENERIC_API, FILE_API };
sdword api_memory[] = { GENERIC_API, MEMORY_API };

handle_t elffile;
int objID;
int launcher_stack[2048];

void launch(void)

{
  CORBA_Environment env = idl4_default_environment;
  Elf32_Ehdr *file_hdr;
  Elf32_Phdr *phdr;
  l4_msgdope_t result;
  char *ptr;
  int size,pagerid;
  int msg[8] = { 0,0x6100,0x0100,0,0,0,0,0 };

  #ifdef DEBUG
  printf("File <%X,%d>, root <%X,%d>, memory <%X,%d>\n",
    elffile.svr.raw,elffile.obj,task[objID].root.svr.raw,task[objID].root.obj,
    task[objID].memory.svr.raw,task[objID].memory.obj);
  printf("Taskid %X, owner %X, cmdline [%s]\n",task[objID].task_id.raw,
    task[objID].owner.raw,task[objID].cmdline);
  #endif

  if (generic_implements(elffile.svr,elffile.obj,sizeof(api_file)/sizeof(int),(sdword*)&api_file, &env)!=EYES)
    enter_kdebug("file api not implemented");

  if (file_open(elffile.svr,elffile.obj,0,&env)!=ESUCCESS)
    enter_kdebug("open fails");

  if (file_read(elffile.svr,elffile.obj,sizeof(Elf32_Ehdr),&size,(char**)&file_hdr,&env)<(int)sizeof(Elf32_Ehdr))
    enter_kdebug("cannot read file header");
    
  if (file_hdr->e_ident[EI_MAG0] !=  ELFMAG0
      || file_hdr->e_ident[EI_MAG1] !=  ELFMAG1
      || file_hdr->e_ident[EI_MAG2] !=  ELFMAG2
      || file_hdr->e_ident[EI_MAG3] !=  ELFMAG3) 
    enter_kdebug("this is NOT an ELF file");  
    
  if (file_hdr->e_type != ET_EXEC) 
    enter_kdebug("unexpected e_type");

  if (file_hdr->e_machine != EM_386) 
    enter_kdebug("not an intel binary");

  if (file_hdr->e_version != EV_CURRENT)
    enter_kdebug("version mismatch?");

  if (file_hdr->e_flags != 0)
    enter_kdebug("unexpected flags?");

  if (file_hdr->e_phnum <= 0)
    enter_kdebug("No loadable program sections");

  // *** create the task. this will map the trampoline code into page 0
  //     of the newly created address space

  #ifdef DEBUG      
  printf("Creating task %d from <%X,%d>...\n",
    nexttask.id.task,elffile.svr.raw,elffile.obj);
  #endif
  
  l4_task_new(nexttask, 255, 0, 0, auxpager);

  if (memory_attach(task[objID].memory.svr,task[objID].memory.obj,&nexttask,&env)!=ESUCCESS)
    enter_kdebug("cannot attach");

  if (memory_get_pagerid(task[objID].memory.svr,task[objID].memory.obj,&pagerid,&env)!=ESUCCESS)
    enter_kdebug("cannot get pager id");
          
  l4_ipc_send(nexttask,0,TRAMPOLINE_NEW_PAGER,pagerid,0,L4_IPC_NEVER,&result);

  task[objID].task_id=nexttask;

  // *** parse all the headers

  for (int i=0;i<file_hdr->e_phnum;i++)
    {
      if (file_seek(elffile.svr,elffile.obj,file_hdr->e_phoff+i*sizeof(Elf32_Phdr),&env)!=ESUCCESS)
        enter_kdebug("P2 seek fail");
      if (file_read(elffile.svr,elffile.obj,sizeof(Elf32_Phdr),&size,(char**)&phdr,&env)<(int)sizeof(Elf32_Phdr))
        enter_kdebug("P2 cannot read phdr");
      if (phdr->p_type == PT_LOAD)
        {	
          if (file_seek(elffile.svr,elffile.obj,phdr->p_offset,&env)!=ESUCCESS)
            enter_kdebug("P3 seek fail");
            
          int remaining = phdr->p_filesz;
          
          while (remaining)
            {
              int todo = (remaining<4096) ? remaining : 4096;
            
              // *** notify the trampoline
 
              l4_ipc_send(nexttask,0,TRAMPOLINE_RECEIVE,
                          (int)phdr->p_vaddr + (phdr->p_filesz - remaining),
                          todo,L4_IPC_NEVER,&result);
	  
              if (file_read(elffile.svr,elffile.obj,todo,&size,&ptr,&env)<todo)
                enter_kdebug("P3 read section");
              if (size<todo)
                enter_kdebug("P3a read");
                
              msg[6]=(int)todo;
              msg[7]=(int)ptr;

              // *** send the string ipc. this will cause a bunch of pagefaults,
              //     which will be handled by the child's pager

              #ifdef DEBUG
              printf("Copying segment of size %x from address %x to %x\n",msg[6],msg[7],(int)phdr->p_vaddr);
              #endif
              l4_ipc_send(nexttask,&msg,0,0,0,L4_IPC_NEVER,&result);
              
              remaining -= todo;
              CORBA_free(ptr);
            }
  
          // *** zero out any bss segments
  
          if (phdr->p_memsz>phdr->p_filesz)
            { 
              int zero_base = phdr->p_vaddr+phdr->p_filesz;
              int zero_size = phdr->p_memsz-phdr->p_filesz;
                
              #ifdef DEBUG
              printf("Erasing zone at %x, size %x\n",zero_base,zero_size);
              #endif
              l4_ipc_send(nexttask,0,TRAMPOLINE_ZERO_ZONE,
                          zero_base,zero_size,L4_IPC_NEVER,&result);
            }
        }
      CORBA_free(phdr);
    }	

  if (file_close(elffile.svr,elffile.obj,&env)!=ESUCCESS)
    enter_kdebug("close fails");

  // *** start the program
      
  #ifdef DEBUG
  printf("Jumping to program entry point at %x [objID=%d]\n\n",(int)file_hdr->e_entry,objID);
  #endif
  l4_ipc_send(nexttask,0,TRAMPOLINE_LAUNCH,objID,file_hdr->e_entry,L4_IPC_NEVER,&result);

  // *** cleanup
  
  CORBA_free(file_hdr);

  // *** wait until thread leaves the trampoline code

  for (int i=0;i<3;i++)
    l4_thread_switch(nexttask);
        
  // *** flush the trampoline  

  l4_fpage_unmap(l4_fpage((int)&_trampoline,L4_LOG2_PAGESIZE,0,0),L4_FP_FLUSH_PAGE);

  (void)nexttask.id.task++;
      
  while (42);  
}

int elf_decode(int xobjID, handle_t xfile)

{
  l4_threadid_t launcher = l4_myself();
  l4_threadid_t dummyid = L4_NIL_ID;
  l4_threadid_t pager = sigma0;
  dword_t foo = L4_INV;

  elffile=xfile;objID=xobjID;
  launcher.id.thread=4;
  
  l4_thread_ex_regs(launcher, (dword_t)launch, (dword_t)&launcher_stack[2047],
                    &dummyid, &pager, &foo, &foo, &foo);
  
  return ESUCCESS;
}

void strip(int objID)

{
  CORBA_Environment env = idl4_default_environment;
  
  if (memory_set_maxpages(task[objID].memory.svr,task[objID].memory.obj,0,&env)!=ESUCCESS)
    enter_kdebug("cannot set maxpages to 0");  
    
  if (memory_unmap(task[objID].memory.svr,task[objID].memory.obj,0x80,&env)!=ESUCCESS)
    enter_kdebug("cannot flush all pages");
}
