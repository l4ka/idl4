#include <stdlib.h>

#include "chacmos.h"
#include "elf.h"
#include "mb_info.h"
#include "interfaces/generic_client.h"
#include "interfaces/file_client.h"
#include "interfaces/memory_client.h"
#include "interfaces/directory_client.h"
#include "interfaces/creator_client.h"
#include "interfaces/booter_server.h"
#include "booter.h"

#define BOOTER_MODULE_NR 2

dword_t pager_stack[THREAD_STACK_SIZE];
task_t task[MAXTASKS];
block_t block[MAXBLOCKS];
handle_t root, dev;
l4_threadid_t booter, auxpager, sigma0, nexttask, memsvr;

int api_mem[] = { GENERIC_API, MEMORY_API };
int api_creator[] = { GENERIC_API, CREATOR_API };
int api_directory[] = { GENERIC_API, FILE_API, DIRECTORY_API };

extern int _edata;
extern int _end;

void init_global_data(void)

{
  l4_threadid_t dummyid = L4_INVALID_ID;
  dword_t foo = L4_INV;

  // *** this is the booter task, and the auxpager will be thread 1

  booter = l4_myself();

  auxpager = booter;
  auxpager.id.thread = 1;

  // *** use ex_regs to read the ID of my pager (should be sigma0)

  sigma0 = L4_INVALID_ID;
  l4_thread_ex_regs(booter, foo, foo, &dummyid, &sigma0, &foo, &foo, &foo);
  
  // *** initialize administrative structs
  
  for (int i=0;i<MAXTASKS;i++)
    task[i].active=0;

  for (int i=0;i<MAXBLOCKS;i++)
    block[i].active=0;
}  

void launch_aux_pager(void)

{
  l4_threadid_t pager = sigma0;
  l4_threadid_t dummyid = L4_NIL_ID;
  dword_t foo = L4_INV;

  // *** Launch the auxiliary pager. Its job is to forward any pagefaults
  //     received from the memory server to sigma0. It also maps the
  //     trampoline page to newly created threads.

  l4_thread_ex_regs(auxpager, (dword_t) pager_server, 
		    (dword_t)&pager_stack[THREAD_STACK_SIZE-1], 
                    &dummyid, &pager, &foo, &foo, &foo);
}                    

void check_multiboot_header(dword_t mb_magic, multiboot_info **mbi)

{
  // *** Find the multiboot header and check sanity. If the header was not
  //     passed in correctly via parameters, try 1M

  if (mb_magic != MULTIBOOT_VALID) 
    {
      mb_magic = *(dword_t *) (1024*1024);
      if (mb_magic == MULTIBOOT_VALID) 
        *mbi = (multiboot_info *)(*(dword_t *)(1024*1024 + 4));
        else enter_kdebug("Failed to find a multiboot header");
    } 

  #ifdef PEDANTIC
  if (!((*mbi)->flags & MB_INFO_MODS))
    enter_kdebug("WARNING: Incorrect flags in multiboot header");
  #endif    
}    

void secure_modules(mod_list *mods, int count)

{
  int foo;

  // *** Touch all the pages needed by the booter task. This is to protect
  //     them from being handed out by sigma0 before we access them.

  for (int i=(int)&_start;i<=(int)&_end;i+=4096) 
    foo = *((volatile int*)i);

  // *** Touch all the pages containing loadable modules. Of course, 
  //     everything up to the booter task is already decoded at this time.
 
  for (int i=0;i<count;i++) 
    {
      int start_page = mods[i].mod_start/4096;
      int end_page = mods[i].mod_end/4096;
      int dummyint;
          
      printf("module %d : %s\n", i, (char *) mods[i].cmdline);
      printf("    start = %x, end = %x\n", (int) mods[i].mod_start,
             (int)mods[i].mod_end);

      if (i>BOOTER_MODULE_NR)
        for (int j=start_page;j<=end_page;j++)
          dummyint = *(volatile int*)(j*4096);
    }
    
  printf("\n");  
}

int main(dword_t mb_magic, struct multiboot_info *mbi)

{
  CORBA_Environment env = idl4_default_environment;
  int i,j,msg[8] = { 0,0x6100,0x0100,0,0,0,0,0 };
  struct mod_list *mods;
  l4_msgdope_t result;
  Elf32_Phdr *phdr;
  l4_threadid_t msvr;
  int mobj, pagerid, tobj;
  int blockID=0;
  int found_root=0;
  int totalBlock=0;
  
  init_global_data();
  printf("Booter task (%x) starting, pager is %X\n", booter.raw, sigma0.raw);

  check_multiboot_header(mb_magic,&mbi);
    
  printf("Multiboot header found at %X\n", (dword_t) mbi);

  mods = (mod_list*) mbi->mods_addr;
  secure_modules(mods, (int)mbi->mods_count);

  launch_aux_pager();

  task[0].active=1;
  task[0].root.svr.raw=0;
  task[0].root.obj=0;
  task[0].memory.svr=sigma0;
  task[0].memory.obj=DEFAULT_OBJECT;
  strncpy(task[0].cmdline,(char*)mods[BOOTER_MODULE_NR].cmdline,MAXLENGTH);
  task[0].task_id=booter;
  task[0].owner=booter;

  // *************************************************************************
  // *** bootup sequence

  if (mbi->mods_count < (BOOTER_MODULE_NR+2)) 
    enter_kdebug("nothing to load");

  mods = (struct mod_list *) mbi->mods_addr;
  nexttask = booter;
  (void)nexttask.id.task++;
 
  for (j=BOOTER_MODULE_NR+1;j<(int)mbi->mods_count;j++)
    {
      Elf32_Ehdr *file_hdr;

      // *** check sanity of elf file
    
      file_hdr = (Elf32_Ehdr *) mods[j].mod_start;
      if (file_hdr->e_ident[EI_MAG0] !=  ELFMAG0
          || file_hdr->e_ident[EI_MAG1] !=  ELFMAG1
          || file_hdr->e_ident[EI_MAG2] !=  ELFMAG2
          || file_hdr->e_ident[EI_MAG3] !=  ELFMAG3) 
        {  
          i=0;
          while ((i<MAXBLOCKS) && (block[i].active)) i++;
          if (i<MAXBLOCKS)
            {
              block[i].active=1;
              block[i].phys_addr=(int)mods[j].mod_start;
              block[i].capacity=((int)mods[j].mod_end-(int)mods[j].mod_start)/512;
              block[i].blocksize=512;
	      totalBlock++;

              if (found_root)
                {
                  char name[20];
                  sprintf(name,"hd%c",'a'+(blockID++));
                  directory_link(dev.svr,dev.obj,3,name,&booter,MAXTASKS+i,&env);
                } 
              
            }
          continue;
        }  
    
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
      
      printf("Task %d: %s\n",(int)nexttask.id.task,(char*)mods[j].cmdline);
      l4_task_new(nexttask, 255, 0, 0, auxpager);

      // *** if the memory server is already running, make it the new task's
      //     pager. (should be replaced by dynamic object creation)
      
      if (j>(BOOTER_MODULE_NR+1))
        {
          if (creator_create(memsvr,DEFAULT_OBJECT,sizeof(api_mem)/sizeof(int),
                             (sdword*)&api_mem,&msvr,&mobj,&env)!=ESUCCESS)
            enter_kdebug("Memory object creation failed");
            
          memory_set_maxpages(msvr,mobj,99999999,&env);
          memory_attach(msvr,mobj,&nexttask,&env);
          memory_get_pagerid(msvr,mobj,&pagerid,&env);
          
          l4_ipc_send(nexttask,0,TRAMPOLINE_NEW_PAGER,pagerid,0,L4_IPC_NEVER,&result);
        } else {
                 msvr = sigma0;mobj=DEFAULT_OBJECT;
               }  

      // *** allocate struct for the task
      
      tobj=0;
      while ((tobj<MAXTASKS) && (task[tobj].active))
        tobj++;
      if (tobj==MAXTASKS)
        enter_kdebug("Too many tasks");
      task[tobj].active=1;
      if (found_root)
        {
          task[tobj].root.svr=root.svr;
          task[tobj].root.obj=root.obj;
        } else {
                 task[tobj].root.svr.raw=0;
                 task[tobj].root.obj=0;
               }  

      task[tobj].memory.svr=msvr;
      task[tobj].memory.obj=mobj;
      strncpy(task[tobj].cmdline,(char*)mods[j].cmdline,MAXLENGTH);
      task[tobj].cmdline[MAXLENGTH-1]=0;
      task[tobj].task_id=nexttask;
      task[tobj].owner=booter;
  
      // *** parse all the headers

      phdr = (Elf32_Phdr *) (file_hdr->e_phoff + (unsigned int) file_hdr);

      for (i=0;i<file_hdr->e_phnum;i++) 
        if (phdr[i].p_type == PT_LOAD) 
          {
            // *** notify the trampoline

            l4_ipc_send(nexttask,0,TRAMPOLINE_RECEIVE,(int)phdr[i].p_vaddr,
                        (int)phdr[i].p_filesz,L4_IPC_NEVER,&result);
            msg[6]=(int)phdr[i].p_filesz;
            msg[7]=(int)file_hdr + phdr[i].p_offset;

            // *** send the string ipc. this will cause a bunch of pagefaults,
            //     which will be handled by the child's pager

            #ifdef DEBUG
            printf("Copying segment of size %x from address %x to %x\n",msg[6],msg[7],(int)phdr[i].p_vaddr);
            #endif
            l4_ipc_send(nexttask,&msg,0,0,0,L4_IPC_NEVER,&result);
  
            // *** zero out any bss segments
  
            if (phdr[i].p_memsz>phdr[i].p_filesz)
              { 
                int zero_base = phdr[i].p_vaddr+phdr[i].p_filesz;
                int zero_size = phdr[i].p_memsz-phdr[i].p_filesz;
                
                #ifdef DEBUG
                printf("Erasing zone at %x, size %x\n",zero_base,zero_size);
                #endif
                l4_ipc_send(nexttask,0,TRAMPOLINE_ZERO_ZONE,
                            zero_base,zero_size,L4_IPC_NEVER,&result);
              }
        }
        
      // *** if this is the memory server, change the pager to sigma0

      if (j==(BOOTER_MODULE_NR+1))
        l4_ipc_send(nexttask,0,TRAMPOLINE_NEW_PAGER,sigma0.raw,0,L4_IPC_NEVER,&result);

      // *** start the program
      
      #ifdef DEBUG
      printf("Jumping to program entry point at %x [objID=%d]\n\n",(int)file_hdr->e_entry,tobj);
      #endif
      l4_ipc_send(nexttask,0,TRAMPOLINE_LAUNCH,tobj,file_hdr->e_entry,L4_IPC_NEVER,&result);

      // *** wait until thread leaves the trampoline code

      for (i=0;i<3;i++)
        l4_thread_switch(nexttask);
        
      // *** flush the trampoline  

      l4_fpage_unmap(l4_fpage((int)&_trampoline,L4_LOG2_PAGESIZE,0,0),L4_FP_FLUSH_PAGE);
      
      // *** see about the supported interfaces
      
      if (j==(BOOTER_MODULE_NR+1))
        {
          memsvr=nexttask; 

          if (generic_implements(nexttask,DEFAULT_OBJECT,sizeof(api_creator)/sizeof(int),(sdword*)&api_creator,&env)!=EYES)
            enter_kdebug("Memory server does not support Creator API");
          if (creator_can_create(nexttask,DEFAULT_OBJECT,sizeof(api_mem)/sizeof(int),(sdword*)&api_mem,&env)!=EYES)
            enter_kdebug("Memory server cannot create memory objects");  
        }
        
      if (!found_root)  
        if (generic_implements(nexttask,DEFAULT_OBJECT,sizeof(api_creator)/sizeof(int),(sdword*)&api_creator,&env)==EYES)
          if (creator_can_create(nexttask,DEFAULT_OBJECT,sizeof(api_directory)/sizeof(int),(sdword*)&api_directory,&env)==EYES)
            {
              root.svr=nexttask;found_root=1;
              #ifdef DEBUG
              printf("Found root nameserver (%X), creating /...\n",root.svr.raw);
              #endif
              if (creator_create(root.svr,DEFAULT_OBJECT,sizeof(api_directory)/sizeof(int),(sdword*)&api_directory,&root.svr,&root.obj,&env)!=ESUCCESS)
                enter_kdebug("Root directory creation failed");
              if (creator_create(root.svr,DEFAULT_OBJECT,sizeof(api_directory)/sizeof(int),(sdword*)&api_directory,&dev.svr,&dev.obj,&env)!=ESUCCESS)
                enter_kdebug("/dev directory creation failed");
              if (generic_implements(root.svr,root.obj,sizeof(api_directory)/sizeof(int),(sdword*)&api_directory,&env)!=EYES)
                enter_kdebug("Defective root directory");
              if (directory_link(root.svr,root.obj,3,"dev",&dev.svr,dev.obj,&env)!=ESUCCESS)
                enter_kdebug("Cannot link dev into /");
              if (directory_link(dev.svr,dev.obj,7,"tasksvr",&booter,0,&env)!=ESUCCESS)
                enter_kdebug("Cannot link tasksvr into /dev/");
              
              for (int i=0;i<MAXBLOCKS;i++)
                if (block[i].active)
                  {
                    char name[20];
                    sprintf(name,"hd%c",'a'+(blockID++));
                    directory_link(dev.svr,dev.obj,3,name,&booter,MAXTASKS+i,&env);
                  }  
              
              for (int k=0;k<MAXTASKS;k++)
                if ((task[k].active) && (!task[k].root.svr.raw))
                  {
                    task[k].root.svr=root.svr;
                    task[k].root.obj=root.obj;
                  }  
              #ifdef DEBUG    
              printf("Root creation completed, all servers notified\n\n");
              #endif
            }  
	    
      (void)nexttask.id.task++;    
    }

  // *** modify the trampoline to work with elf launcher
  // *** this is BAD magic!
  
  int *m1=(int*)(((int)&_m1)+1);
  int *m2=(int*)(((int)&_m2)+1);
  *m1=0x04041001;
  *m2=0x04041001;

  if (totalBlock)
    printf("Created %d block device(s)\n",totalBlock);

  // *** enter server loop

  booter_server();  
}
