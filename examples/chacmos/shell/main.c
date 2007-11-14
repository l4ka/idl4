#include <stdlib.h>

#include "chacmos.h"
#include "interfaces/constants_client.h"
#include "interfaces/generic_client.h"
#include "interfaces/task_client.h"
#include "interfaces/file_client.h"
#include "interfaces/directory_client.h"
#include "interfaces/block_client.h"
#include "interfaces/creator_client.h"
#include "interfaces/memory_client.h"
#include "interfaces/filesystem_client.h"

sdword api_task[] = { GENERIC_API, TASK_API };
sdword api_directory[] = { GENERIC_API, FILE_API, DIRECTORY_API };
sdword api_memory[] = { GENERIC_API, MEMORY_API };
sdword api_block[] = { GENERIC_API, BLOCK_API };
sdword api_file[] = { GENERIC_API, FILE_API };
sdword api_filesys[] = { GENERIC_API, FILESYSTEM_API };
sdword api_lockable[] = { GENERIC_API, LOCKABLE_API };
sdword api_creator[] = { GENERIC_API, CREATOR_API };

handle_t root, pwd, mytask;

char pwdname[200];
int debug=0;

L4_INLINE void cursor(int x, int y)

{
  asm (
        "int $3\n\t"
        "cmpb $4,%%al\n\t"
        : :"a" (x<<8+y) );
}        

int init_root(void)

{
  CORBA_Environment env = idl4_default_environment;
  
  if (generic_implements(mytask.svr,mytask.obj,sizeof(api_task)/sizeof(int),(sdword*)&api_task,&env)!=EYES)
    return 0;
  if (task_get_root(mytask.svr,mytask.obj,&root.svr,&root.obj,&env)!=ESUCCESS)
    return 0;
  if (generic_implements(root.svr,root.obj,sizeof(api_directory)/sizeof(int),(sdword*)&api_directory,&env)!=EYES)  
    return 0;

  pwd=root;strcpy(pwdname,"/");
     
  return 1;  
}

void inputline(char *buf, int maxlen)

{
  int cpos=0,len=0,ch,i;

  strcpy(buf,"#");
    
  do {
       printf("\rchacmOS:%s > %s ",pwdname,buf);
       
       ch=getc();
       
       if (ch=='z') ch='y'; else 
       if (ch=='y') ch='z';
       
       switch (ch)
         {
           case 127 : if (cpos>0) 
                        {
                          strcpy(&buf[cpos-1],&buf[cpos]);
                          len--;cpos--;
                        }  
                      break;
           case  52 : if (cpos) 
                        { 
                          buf[cpos]=buf[cpos-1];buf[cpos-1]='#';
                          cpos--;
                        }  
                      break;
           case  54 : if (cpos<len) 
                        {
                          buf[cpos]=buf[cpos+1];buf[cpos+1]='#';
                          cpos++;
                        }  
                      break;
           case  27 : cpos=0;len=0;strcpy(buf,"#");
                      break;
           case  13 : strcpy(&buf[cpos],&buf[cpos+1]);
                      break;
           default  : if (len<maxlen)
                        {
                          for (i=len+1;i>=cpos;i--)
                            buf[i+1]=buf[i];
                          len++;buf[cpos++]=ch;
                        } 
                      break;
         }             
     } while (ch!=13);
  printf("\rchacmOS:%s > %s \n",pwdname,buf);
}

void listdir(handle_t dir)

{
  CORBA_Environment env = idl4_default_environment;
  int len,size,x,cnt;
  char *buf;

  if (!dir.svr.raw) return;
 
  if (debug) 
    printf("ls: showing content of <%X,%d>\n",dir.svr.raw,dir.obj);
  
  if (generic_implements(dir.svr,dir.obj,sizeof(api_directory)/sizeof(int),(sdword*)&api_directory,&env)!=EYES)
    {
      printf("ls: not a directory\n");
      return;
    }  
  if (file_open(dir.svr,dir.obj,0,&env)!=ESUCCESS)
    enter_kdebug("cannot open directory");
  if (file_seek(dir.svr,dir.obj,-1,&env)!=ESUCCESS)
    enter_kdebug("cannot seek to end of directory");
  if (file_tell(dir.svr,dir.obj,&len,&env)!=ESUCCESS)
    enter_kdebug("cannot tell file size");
    
  if (debug) printf("fileserver tells me directory has %d bytes\n",len);
  
  if (file_seek(dir.svr,dir.obj,0,&env)!=ESUCCESS)
    enter_kdebug("cannot reseek to beginning");
  
  if (file_read(dir.svr,dir.obj,len,&size,&buf,&env)<=0)
    enter_kdebug("cannot read content of directory");
    
  if (file_close(dir.svr,dir.obj,&env)!=ESUCCESS)
    enter_kdebug("cannot close directory");

  x=0;cnt=0;buf[len]=0;
  while (x<len)
    {
      printf("%-19s",&buf[x]);
      while (buf[x++]);
      cnt++;
      if (!(cnt%4)) printf("\n");
    }
  if (cnt%4) 
    printf("\n");
  printf("%d file(s)\n\n",cnt);    
  
  CORBA_free(buf);
}

void detaileddir(handle_t dir)

{
  CORBA_Environment env = idl4_default_environment;
  int len,size,x,y,z,cnt,details;
  l4_threadid_t t;
  handle_t file;
  char *buf;

  if (!dir.svr.raw) return;

  if (debug) 
    printf("dir: showing content of <%X,%d>\n",dir.svr.raw,dir.obj);
  
  if (generic_implements(dir.svr,dir.obj,sizeof(api_directory)/sizeof(int),(sdword*)&api_directory,&env)!=EYES)
    {
      printf("dir: not a directory\n");
      return;
    }  
  if (file_open(dir.svr,dir.obj,0,&env)!=ESUCCESS)
    enter_kdebug("cannot open directory");
  if (file_seek(dir.svr,dir.obj,-1,&env)!=ESUCCESS)
    enter_kdebug("cannot seek to end of directory");
  if (file_tell(dir.svr,dir.obj,&len,&env)!=ESUCCESS)
    enter_kdebug("cannot tell file size");
    
  if (debug) printf("fileserver tells me directory has %d bytes\n",len);
  
  if (file_seek(dir.svr,dir.obj,0,&env)!=ESUCCESS)
    enter_kdebug("cannot reseek to beginning");
  
  if (file_read(dir.svr,dir.obj,len,&size,&buf,&env)<=0)
    enter_kdebug("cannot read content of directory");
  if (file_close(dir.svr,dir.obj,&env)!=ESUCCESS)
    enter_kdebug("cannot close directory");

  x=0;cnt=0;buf[len]=0;
  while (x<len)
    {
      printf("%-19s",&buf[x]);
      if (directory_resolve(dir.svr,dir.obj,strlen(&buf[x]),&buf[x],&file.svr,&file.obj,&env)!=ESUCCESS)
        enter_kdebug("cannot resolve this file");
      printf("<%X,%3d>  ",file.svr.raw,file.obj);details=0;

      if (generic_implements(file.svr,file.obj,sizeof(api_task)/sizeof(int),(sdword*)&api_task,&env)==EYES)
        { printf("T");details=1; } else printf("-");
      if (generic_implements(file.svr,file.obj,sizeof(api_file)/sizeof(int),(sdword*)&api_file,&env)==EYES)
        { printf("F");details=2; } else printf("-");
      if (generic_implements(file.svr,file.obj,sizeof(api_directory)/sizeof(int),(sdword*)&api_directory,&env)==EYES)
        { printf("D");details=2; } else printf("-");
      if (generic_implements(file.svr,file.obj,sizeof(api_filesys)/sizeof(int),(sdword*)&api_filesys,&env)==EYES)
        printf("S"); else printf("-");
      if (generic_implements(file.svr,file.obj,sizeof(api_creator)/sizeof(int),(sdword*)&api_creator,&env)==EYES)
        printf("C"); else printf("-");
      if (generic_implements(file.svr,file.obj,sizeof(api_memory)/sizeof(int),(sdword*)&api_memory,&env)==EYES)
        printf("M"); else printf("-");
      if (generic_implements(file.svr,file.obj,sizeof(api_block)/sizeof(int),(sdword*)&api_block,&env)==EYES)
        { printf("B");details=3; } else printf("-");
      if (generic_implements(file.svr,file.obj,sizeof(api_lockable)/sizeof(int),(sdword*)&api_lockable,&env)==EYES)
        printf("L"); else printf("-");
	
      printf("  ");
      switch (details)
        {
       	  case 1 : if (task_get_threadid(file.svr,file.obj,&t,&env)!=ESUCCESS)
	             enter_kdebug("failed to get thread ID");
		   printf("TID=%X\n",t.raw);
		   break; 
	  case 2 : if (file_open(file.svr,file.obj,0,&env)!=ESUCCESS)
	             enter_kdebug("cannot open this file");
		   if (file_seek(file.svr,file.obj,-1,&env)!=ESUCCESS)
		     enter_kdebug("cannot seek");
		   if (file_tell(file.svr,file.obj,&y,&env)!=ESUCCESS)
		     enter_kdebug("won't tell");
		   if (file_close(file.svr,file.obj,&env)!=ESUCCESS)
		     enter_kdebug("cannot close file");  
		   printf("%7d bytes\n",y);
		   break;
	  case 3 : if (block_get_capacity(file.svr,file.obj,&z,&env)!=ESUCCESS)
	             enter_kdebug("cannot get capacity");
		   if (block_get_blocksize(file.svr,file.obj,&y,&env)!=ESUCCESS)
		     enter_kdebug("cannot get blocksize");
		   printf("%7d bytes (%d blocks)\n",z*y,z);
		   break;
	  default: printf("\n");
	}   	       
	
      while (buf[x++]);
      cnt++;
    }
  printf("%d file(s)\n\n",cnt);    
  
  CORBA_free(buf);
}

void chdir(char *abspath)

{
  CORBA_Environment env = idl4_default_environment;
  char *next = abspath;
  handle_t pos = root;
  char newpath[100];

  strcpy(newpath,"/");
  if (*next=='/') next++;
  next=strtok(next,"/");

  while (next!=NULL)
    {
      switch (directory_resolve(pos.svr,pos.obj,strlen(next),next,&pos.svr,&pos.obj,&env))
        {
          case ESUCCESS  : break;
          case ENOTFOUND : printf("cd: directory '%s' does not exist\n",next);
                           return;
          default        : printf("cd: unknown error when changing to '%s'\n",next);
                           return;
        }       
        
      switch (generic_implements(pos.svr,pos.obj,sizeof(api_directory)/sizeof(int),(sdword*)&api_directory,&env))
        {
          case EYES      : break;
          default        : printf("cd: %s: not a directory\n",next);
                           return;
        }

      strcat(newpath,next);
      strcat(newpath,"/");
      next=strtok(NULL,"/");
    }  
    
  strcpy(pwdname,newpath);
  pwd=pos;
}

void splitpath(char *composite, char *path, char *fname)

{
  char *lastslash, *p2;
  
  lastslash=NULL;p2=composite;
  while (*p2)
    {
      if (*p2=='/') lastslash=p2;
      p2++;
    }
    
  if (lastslash!=NULL)
    {
      strncpy(path,composite,1+(int)(lastslash-composite));
      strcpy(fname,lastslash+1);
    } else {
             strcpy(path,"");
             strcpy(fname,composite);
           }  
}           

char *abspath(char *relpath)

{
  char newpath[100];
  char *pos,*p2,*path = relpath;
  
  if (debug) printf("curdir=[%s] relpath=[%s]",pwdname,path);
  
  if (*path=='/')
    {
      strcpy(newpath,"/");
      path++;
    } else strcpy(newpath,pwdname);
    
  while (*path)    
    {
      pos=path;
      while ((*path) && (*path!='/')) path++;
      if (*path=='/') *path++=0;

      if (!strcmp(pos,".."))
        {
          p2=newpath;
          while (*(++p2));
          p2--;
          if (p2!=newpath) // in case this already is root
            while (*(--p2)!='/');
          *(++p2)=0;
        } else

      if (strcmp(pos,"."))
        {
          strcat(newpath,pos);strcat(newpath,"/");
        }    
    }
  
  if (debug) printf(" abspath=[%s]\n",newpath);
  
  strcpy(relpath,newpath);
  
  return relpath;
}  

handle_t resolve(char *name)

{
  CORBA_Environment env = idl4_default_environment;
  char dir[80],file[20],*path=dir,*next;
  handle_t pos=root,nil;
 
  splitpath(name,dir,file);
  
  abspath(dir);
  strcpy(name,dir);strcat(name,file);
  nil.svr.raw=0;nil.obj=0;

  // separate 
  
  if (debug) printf("we have [%s] [%s]\n",dir,file);

  next=strtok((*path=='/') ? path+1 : path,"/");

  while (next!=NULL)
    {
      switch (directory_resolve(pos.svr,pos.obj,strlen(next),next,&pos.svr,&pos.obj,&env))
        {
          case ESUCCESS  : break;
          case ENOTFOUND : printf("resolve: directory '%s' does not exist\n",next);
                           return nil;
          default        : printf("resolve: unknown error when changing to '%s'\n",next);
                           return nil;
        }       
        
      switch (generic_implements(pos.svr,pos.obj,sizeof(api_directory)/sizeof(int),(sdword*)&api_directory,&env))
        {
          case EYES      : break;
          default        : printf("resolve: %s: not a directory\n",next);
                           return nil;
        }

      next=strtok(NULL,"/");
    }   
    
  if (debug) printf("resolved dir to <%X,%d>\n",pos.svr.raw,pos.obj);
   
  if (file[0]) 
    switch (directory_resolve(pos.svr,pos.obj,strlen(file),file,&pos.svr,&pos.obj,&env))
      {
        case ESUCCESS  : break;
        case ENOTFOUND : printf("resolve: file '%s' not found\n",file);
                         return nil;
        default        : printf("resolve: unknown error when resolving '%s'\n",file);
                         return nil;
      }       
   
  return pos;
}

void cat(char *name)

{
  CORBA_Environment env = idl4_default_environment;
  handle_t file;
  char *buf;
  int read,size;

  file = resolve(name);
  if (debug) printf("cat: resolved %s to <%X,%d>\n",name,file.svr.raw,file.obj);

  if (!file.svr.raw)
    {
      printf("cat: cannot find '%s'\n",name);
      return;
    }       

  switch (generic_implements(file.svr,file.obj,sizeof(api_file)/sizeof(int),(sdword*)&api_file,&env))
    { 
      case EYES      : break;
      case ENO       : printf("cat: %s: not a file\n",name);
                       return;
      default        : enter_kdebug("unexpected error cat.2");                 
    }  

  switch (file_open(file.svr,file.obj,0,&env))  
    {
      case ESUCCESS  : break;
      case ELOCKED   : printf("cat: %s: locked by another thread\n",name);
                       return;                
      case EDENIED   : printf("cat: %s: access denied\n",name);
                       break;
      default        : enter_kdebug("unexpected error cat.3");
    }  

  read=file_read(file.svr,file.obj,16,&size,&buf,&env);
  while ((read>0) && (read<=16))
    {
      buf[read]=0;
      for (int i=0;i<read;i++) if (buf[i]==0x0A) printf("\r\n"); else printf("%c",buf[i]);
      CORBA_free(buf);
      read=file_read(file.svr,file.obj,16,&size,&buf,&env);
    }
  CORBA_free(buf);
     
  switch (read)
    {
      case EEOF      : break;
      case ELOCKED   : printf("cat: %s: locked by another thread\n",name);
                       return;
      default        : enter_kdebug("unexpected error cat.4");
    }  
  printf("\n");     
   
  switch (file_close(file.svr,file.obj,&env))
    {
      case ESUCCESS  : break;
      default        : enter_kdebug("unexpected error cat.5");
    }  
}

void execute(char *cmd, char *param)

{
  CORBA_Environment env = idl4_default_environment;
  handle_t executable;
  handle_t memsvr, memory;
  handle_t task;

  if (param==NULL) param="";
  
  if (debug) printf("Executing [%s] with a cmdline of [%s]\n",cmd,param);

  switch (directory_resolve(pwd.svr,pwd.obj,strlen(cmd),cmd,&executable.svr,&executable.obj,&env))
    {
      case ESUCCESS  : break;
      case ENOTFOUND : printf("shell: cannot find executable '%s'\n",cmd);
                       return;
      default        : enter_kdebug("unexpected error exec.1");
                       return;
    }       
  
  if (debug) printf("resolved file to <%X,%d>\n",executable.svr.raw,executable.obj);
    
  switch (generic_implements(executable.svr,executable.obj,sizeof(api_file)/sizeof(int),(sdword*)&api_file,&env))
    { 
      case EYES      : break;
      case ENO       : printf("exec: %s: not a file\n",cmd);
                       return;
      default        : enter_kdebug("unexpected error cat.2");                 
    }  

  if (generic_implements(mytask.svr,mytask.obj,sizeof(api_creator)/sizeof(int),(sdword*)&api_creator,&env)!=EYES)
    enter_kdebug("no creator api");
    
  if (creator_can_create(mytask.svr,mytask.obj,sizeof(api_task)/sizeof(int),(sdword*)&api_task,&env)!=EYES)
    enter_kdebug("he can't create tasks");

  if (creator_create(mytask.svr,mytask.obj,sizeof(api_task)/sizeof(int),(sdword*)&api_task,&task.svr,&task.obj,&env)!=ESUCCESS)
    enter_kdebug("create failed");

  if (task_get_memory(mytask.svr,mytask.obj,&memsvr.svr,&memsvr.obj,&env)!=ESUCCESS)
    enter_kdebug("cannot get memsvr");
    
  if (creator_can_create(memsvr.svr,DEFAULT_OBJECT,sizeof(api_memory)/sizeof(int),(sdword*)&api_memory,&env)!=EYES)
    enter_kdebug("he can't create memory");

  if (creator_create(memsvr.svr,DEFAULT_OBJECT,sizeof(api_memory)/sizeof(int),(sdword*)&api_memory,&memory.svr,&memory.obj,&env)!=ESUCCESS)
    enter_kdebug("create failed");
    
  if (memory_set_maxpages(memory.svr,memory.obj,9999999,&env)!=ESUCCESS)
    enter_kdebug("set maxpages failed");
    
  if (task_set_memory(task.svr,task.obj,&memory.svr,memory.obj,&env)!=ESUCCESS)
    enter_kdebug("set memory failed");
    
  if (task_set_root(task.svr,task.obj,&root.svr,root.obj,&env)!=ESUCCESS)
    enter_kdebug("set root failed");
    
  if (task_set_cmdline(task.svr,task.obj,strlen(param),param,&env)!=ESUCCESS)
    enter_kdebug("cannot set cmdline");
    
  if (task_execute(task.svr,task.obj,&executable.svr,executable.obj,&env)!=ESUCCESS)
    enter_kdebug("start failed");
}

void mount(char *blockdevice, char *filesystem)

{
  CORBA_Environment env = idl4_default_environment;
  char absbdev[50], fsysdev[50];
  
  strcpy(absbdev,blockdevice);
  strcpy(fsysdev,filesystem);
  
  handle_t bdev = resolve(absbdev);
  handle_t fsys = resolve(fsysdev);

  if (!bdev.svr.raw) 
    {
      printf("mount: cannot resolve %s\n",blockdevice);
      return;
    }
  if (!fsys.svr.raw)
    {
      printf("mount: cannot resolve %s\n",filesystem);
      return;
    }    

  if (debug) printf("mounting <%X,%d> into <%X,%d>\n",bdev.svr.raw,bdev.obj,fsys.svr.raw,fsys.obj);    
  
  switch (generic_implements(bdev.svr,bdev.obj,sizeof(api_block)/sizeof(int),(sdword*)&api_block,&env))
    { 
      case EYES      : break;
      case ENO       : printf("mount: %s: not a block device\n",blockdevice);
                       return;
      default        : enter_kdebug("unexpected error mount.1");                 
    }  

  switch (generic_implements(fsys.svr,fsys.obj,sizeof(api_filesys)/sizeof(int),(sdword*)&api_filesys,&env))
    { 
      case EYES      : break;
      case ENO       : printf("mount: %s: not a file system\n",filesystem);
                       return;
      default        : enter_kdebug("unexpected error mount.2");                 
    }  

  switch (filesystem_mount(fsys.svr,fsys.obj,&bdev.svr,bdev.obj,&env))
    { 
      case ESUCCESS  : break;
      case EINVALID  : printf("mount: %s rejected by %s\n",blockdevice,filesystem);
                       return;
      case ENOTFOUND : printf("mount: media not recognized by %s\n",filesystem);
                       return;
      case EEXISTS   : printf("mount: %s is already mounted\n",filesystem);
                       return;
      default        : enter_kdebug("unexpected error mount.3");
    }  
}  

void umount(char *filesystem)

{
  CORBA_Environment env = idl4_default_environment;
  char fsysdev[50];
  
  strcpy(fsysdev,filesystem);
  
  handle_t fsys = resolve(fsysdev);

  if (!fsys.svr.raw)
    {
      printf("umount: cannot resolve %s\n",filesystem);
      return;
    }    

  if (debug) printf("unmounting <%X,%d>\n",fsys.svr.raw,fsys.obj);    
  
  switch (generic_implements(fsys.svr,fsys.obj,sizeof(api_filesys)/sizeof(int),(sdword*)&api_filesys,&env))
    { 
      case EYES      : break;
      case ENO       : printf("umount: %s: not a file system\n",filesystem);
                       return;
      default        : enter_kdebug("unexpected error umount.1");
    }  

  switch (filesystem_unmount(fsys.svr,fsys.obj,&env))
    { 
      case ESUCCESS  : break;
      case ENOTFOUND : printf("umount: %s is not mounted\n",filesystem);
                       return;
      default        : enter_kdebug("unexpected error umount.2");
    }  
    
  if (generic_implements(pwd.svr,pwd.obj,sizeof(api_directory)/sizeof(int),(sdword*)&api_directory,&env)!=EYES)
    {
      pwd=root;
      strcpy(pwdname,"/");
      printf("umount: changing pwd to /\n");
    }  
}  

void credits(void)

{
  printf("ChacmOS authors:\n");
  printf("  Andreas Haeberlen, Christian Schwarz, Horst Wenske, Markus Völp\n");
} 

void test(void)

{
}

int main(l4_threadid_t tasksvr, int taskobj)

{
  char linebuf[100];
  char s[100];
  char *cmd,*p;

  mytask.svr=tasksvr;mytask.obj=taskobj;
  
  if (!init_root())
    enter_kdebug("cannot init root");
    
  printf("\n=======================================================\n");
  printf("chacmOS V1.00 - built on %s %s\n\n",__DATE__,__TIME__);
 
  do {  
       do { inputline(linebuf,80); } while (!linebuf[0]);
       
       cmd=strtok(linebuf," ");
       
       if (debug) printf("shell: executing command '%s'\n",cmd);
       
       // ls
       
       if (!strcmp(cmd,"ls"))
         {
           char *where = strtok(NULL," ");
           if (where!=NULL)
             {
               strcpy(s,where);
               listdir(resolve(s));
             } else listdir(pwd); 
         } else    
         
       // dir
       
       if (!strcmp(cmd,"dir"))
         {
           char *where = strtok(NULL," ");
           if (where!=NULL)
             {
               strcpy(s,where);
               detaileddir(resolve(s));
             } else detaileddir(pwd); 
         } else    
	 
       // cd  
         
       if (!strcmp(cmd,"cd"))
         {
           p=strtok(NULL," ");
           if (p!=NULL)
             {
               strcpy(s,p);
               p=s;while (*p) p++;
               if (p!=s) p--;
               if (*p!='/') strcat(s,"/");
             } else strcpy(s,"/");
             
           handle_t newdir = resolve(s);
           
           if (newdir.svr.raw) 
             {
               strcpy(pwdname,s);
               pwd=newdir;
             }  
             
         } else  
         
       // pwd  
         
       if (!strcmp(cmd,"pwd"))
         printf("%s\n",pwdname); else  

       // test
         
       if (!strcmp(cmd,"test"))
         test(); else  

       // credits
         
       if (!strcmp(cmd,"credits"))
         credits(); else  
         
       // kd  
         
       if (!strcmp(cmd,"kd"))
         enter_kdebug("hit 'g' to continue"); else
         
       // debug  
         
       if (!strcmp(cmd,"debug"))
         {
           char *arg=strtok(NULL," ");
           if (arg!=NULL)
             {
               if (!strcmp(arg,"on")) debug=1;  
               if (!strcmp(arg,"off")) debug=0;
             }  
           printf("debug: ");
           if (debug) printf("enabled\n"); else printf("disabled\n");
         } else  
         
       // help  
         
       if (!strcmp(cmd,"help"))
         {
           printf("Supported commands:\n");
           printf("  cat <file>         displays contents of <file>\n");
           printf("  cd <path>          changes working directory to <path>\n");
           printf("  debug [on|off]     enables or disables debugging\n");
	   printf("  dir                shows detailed information about directory\n");
           printf("  help               displays the message you're reading\n");
           printf("  kd                 enters kernel debugger\n");
           printf("  ls                 lists contents of working directory\n");
           printf("  mount <bdev> <fs>  mounts a block device into a file system\n");
           printf("  pwd                prints path to working directory\n");
	   printf("  umount <fs>        unmounts the filesystem\n");
         } else
         
       // cat  
         
       if (!strcmp(cmd,"cat"))
         cat(strtok(NULL," ")); else

       // mount
       
       if (!strcmp(cmd,"mount"))
         {
           char *bdev = strtok(NULL," ");
           char *fsys = strtok(NULL,"\n");
           
           if (bdev==NULL)
             printf("mount: missing block device\n"); else
           if (fsys==NULL)
             printf("mount: missing file server\n"); else
             mount(bdev,fsys);
         } else

       // umount
       
       if (!strcmp(cmd,"umount"))
         {
           char *fsys = strtok(NULL," ");
           
           if (fsys==NULL)
             printf("umount: missing file system\n"); else
             umount(fsys);
         } else
         
       // user command  
         
       execute(cmd,strtok(NULL,"\n"));
       
     } while (1);
}
