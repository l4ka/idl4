#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "getopt_gnu.h"
#endif

#include "globals.h"
#include "base.h"
#include "fe/idl.h"
#include "aoi.h"
#include "check.h"
#include "dump.h"
#include "ms.h"
#include "cross.h"
#include "be.h"
#include "cast.h"

#include "arch/x0.h"
#include "arch/dummy.h"
#include "arch/v2.h"
#include "arch/v4.h"

#define dprintln(fmt...) do { if (debug_mode&(~(DEBUG_TESTSUITE+DEBUG_PARANOID))) printf(fmt); } while (0)
#define OPTION(sopt, lopt, arg, expl) printf("  -%c%-21s%s\n", sopt, (lopt[0]) ? mprintf(", --%s%s%s", lopt, (arg[0] ? "=" : " "), arg) : "", expl)

global_t globals = { {0}, {0}, {0}, {0}, {0}, {0}, {0}, idl4_cpp, "", NULL, NULL, OUTPUT_CLIENT, PLATFORM_IA32, INTERFACE_X0, MAPPING_C, FLAG_USEMALLOC|FLAG_NOEMPTYFILES, 0, 4, false };
CMSFactory *msFactory = NULL;
CAoiFactory *aoiFactory = NULL;
CAoiRootScope *aoiRoot = NULL;
int debug_mode = 0;

void help(void)

{
  printf("Usage: idl4 [OPTIONS]... IDL_FILE\n");
  printf("Generate stub code for the L4 microkernel platform\n\n");
  OPTION('c',""         ,""    ,"generate client stubs");
  OPTION('d',"debug"    ,"MODE","show verbose debug output");
  OPTION('h',"header"   ,"FILE","rename output header file");
  OPTION('o',"output"   ,"FILE","use different name for implementation file");
  OPTION('s',""         ,""    ,"generate server stubs");
  OPTION('t',""         ,""    ,"produce template");
  OPTION('w',"word-size","BITS","set word size");
  OPTION('C',"compat"   ,""    ,"enable compatibility between architectures");
  OPTION('f',""         ,""    ,"supply flags to the compiler");
  OPTION('D',""         ,""    ,"preprocessor definition");
  OPTION('I',""         ,""    ,"specifiy additional include directory");
  OPTION('W',""         ,""    ,"enable specific warnings");
  OPTION('v',"version"  ,""    ,"display version information");
  OPTION('p',"platform" ,"SPEC","specify platform (ia32, arm)");
  OPTION('i',"interface","SPEC","choose kernel interface (V2, X0, V4)");
  OPTION('m',"mapping"  ,"SPEC","use specific mapping (C)");
  printf("\n");
  exit(1);
}

void version(void)

{
  printf("IDL4 %s - built on %s by %s\n", idl4_version, idl4_build, idl4_user);
  exit(1);
}

void parse_args(int argc, char* argv[])

{
  char optch;
  static char stropts[] = "stco:h:d::vp:i:m:f:D:I:W:w:C:";
  int optidx;
  static struct option longopts[] = 
    { { "debug",     optional_argument, 0, 'd' },
      { "version",   no_argument,       0, 'v' },
      { "output",    required_argument, 0, 'o' },
      { "platform",  required_argument, 0, 'p' },
      { "header",    required_argument, 0, 'h' },
      { "interface", required_argument, 0, 'i' },
      { "mapping",   required_argument, 0, 'm' },
      { "word-size", required_argument, 0, 'w' },
      { "compat",    no_argument,       0, 'C' },
      { "with-cpp",  required_argument, 0, 2   },
      { "sys-prefix",required_argument, 0, 3   },
      { "pre-call",  required_argument, 0, 4   },
      { "post-call", required_argument, 0, 5   },
      { "help",      no_argument,       0, 1   },
      { 0,           0,                 0, 0   } };

  for (int i=0;i<MAX_OPTIONS;i++)
    {
      globals.preproc_includes[i] = NULL;
      globals.preproc_options[i] = NULL;
    }

  for (int i=0;i<MAX_OPTIONS;i++)
    globals.preproc_defines[i] = getBuiltinMacro(i);

  while ((optch=getopt_long(argc, argv, stropts, longopts, &optidx))!=EOF)
    switch (optch)
      {
        case 's' : globals.output_type = OUTPUT_SERVER; break;
        case 't' : globals.output_type = OUTPUT_TEMPLATE; break;
        case 'c' : globals.output_type = OUTPUT_CLIENT; break;
        case 'o' : strncpy(globals.outfile_name, optarg, FILENAME_MAX);break;
        case 'h' : strncpy(globals.header_name, optarg, FILENAME_MAX);break;
        case 'v' : version();break;
        case 'd' : if (optarg)
                     { 
                       if (!strcasecmp(optarg, "test"))
                         debug_mode |= DEBUG_TESTSUITE; else
                       if (!strcasecmp(optarg, "paranoid"))
                         debug_mode |= DEBUG_PARANOID; else
                       if (!strcasecmp(optarg, "cpp"))
                         debug_mode |= DEBUG_CPP; else
                       if (!strcasecmp(optarg, "aoi"))
                         debug_mode |= DEBUG_AOI; else
                       if (!strcasecmp(optarg, "check"))
                         debug_mode |= DEBUG_CHECK; else
                       if (!strcasecmp(optarg, "cast"))
                         debug_mode |= DEBUG_CAST; else
                       if (!strcasecmp(optarg, "fids"))
                         debug_mode |= DEBUG_FIDS; else
                       if (!strcasecmp(optarg, "marshal"))
                         debug_mode |= DEBUG_MARSHAL; else
                       if (!strcasecmp(optarg, "generator"))
                         debug_mode |= DEBUG_GENERATOR; else
                       if (!strcasecmp(optarg, "import"))
                         debug_mode |= DEBUG_IMPORT; else
                       if (!strcasecmp(optarg, "reorder"))
                         debug_mode |= DEBUG_REORDER; else
                       if (!strcasecmp(optarg, "visual"))
                         debug_mode |= DEBUG_VISUAL;
                           else {
                                  printf("Unknown debug mode: %s\n", optarg);
                                  exit(1);
                                }  
                     } else debug_mode = ~0;
                   break;
        case 'W' : if (optarg[1]==',')
                     {
                       switch (optarg[0])
                         {
                           case 'p' : {
                                        int i = 0;
                                        while (i<MAX_OPTIONS && globals.preproc_options[i]) i++;
                                        if (i==MAX_OPTIONS)
                                          panic("Too many pass-through options (max %d allowed)", MAX_OPTIONS);
                                        globals.preproc_options[i] = optarg+2;
                                        break;
                                      }  
                           default  : printf("Malformed pass-through option: %s\n", optarg);
                                      exit(1);
                         }
                     } else
                   if (!strcasecmp(optarg, "prealloc"))
                     globals.warnings |= WARN_PREALLOC; else
                   if (!strcasecmp(optarg, "all"))
                     globals.warnings = ~0; else
                       {
                         printf("Unknown warning: %s\n", optarg);
                         exit(1);
                       }  
                   break;
        case 'D' : {
                     int i = 0;
                     while (i<MAX_OPTIONS && globals.preproc_defines[i]) i++;
                     if (i==MAX_OPTIONS)
                       panic("Too many preprocessor defines (max %d allowed)", MAX_OPTIONS);
                     globals.preproc_defines[i] = optarg;
                   }  
                   break;
        case 'I' : {
                     int i = 0;
                     while (i<MAX_OPTIONS && globals.preproc_includes[i]) i++;
                     if (i==MAX_OPTIONS)
                       panic("Too many include paths (max %d allowed)", MAX_OPTIONS);
                     globals.preproc_includes[i] = optarg;
                   }  
                   break;
        case 'f' : if (!strcasecmp(optarg, "fastcall"))
                     globals.flags |= FLAG_FASTCALL; else
                   if (!strcasecmp(optarg, "no-fastcall"))
                     globals.flags &= ~FLAG_FASTCALL; else
                   if (!strcasecmp(optarg, "ctypes"))
                     globals.flags |= FLAG_CTYPES; else  
                   if (!strcasecmp(optarg, "no-ctypes"))
                     globals.flags &= ~FLAG_CTYPES; else  
                   if (!strcasecmp(optarg, "lipc"))
                     globals.flags |= FLAG_LIPC; else  
                   if (!strcasecmp(optarg, "no-lipc"))
                     globals.flags &= ~FLAG_LIPC; else  
                   if (!strcasecmp(optarg, "omit-frame-pointer"))
                     globals.flags |= FLAG_NOFRAMEPTR; else  
                   if (!strcasecmp(optarg, "no-omit-frame-pointer"))
                     globals.flags &= ~FLAG_NOFRAMEPTR; else
                   if (!strcasecmp(optarg, "omit-empty-files"))
                     globals.flags |= FLAG_NOEMPTYFILES; else  
                   if (!strcasecmp(optarg, "no-omit-empty-files"))
                     globals.flags &= ~FLAG_NOEMPTYFILES; else
                   if (!strcasecmp(optarg, "use-malloc"))
                     globals.flags |= FLAG_USEMALLOC; else  
                   if (!strcasecmp(optarg, "no-use-malloc"))
                     globals.flags &= ~FLAG_USEMALLOC; else
                   if (!strcasecmp(optarg, "loop-only"))
                     globals.flags |= FLAG_LOOPONLY; else  
                   if (!strcasecmp(optarg, "modules-only"))
                     globals.flags |= FLAG_MODULESONLY; else  
                       {
                         printf("Unknown flag: %s\n", optarg);
                         exit(1);
                       }
                   break;
        case 'i' : if (!strcasecmp(optarg,"V2")) globals.interface = INTERFACE_V2; else
                   if (!strcasecmp(optarg,"X0")) globals.interface = INTERFACE_X0; else
                   if (!strcasecmp(optarg,"V4")) globals.interface = INTERFACE_V4; else
                     {
                       printf("Unknown kernel interface: %s\n", optarg);
                       exit(1);
                     }
                   break;    
        case 'm' : if (!strcasecmp(optarg,"C")) globals.mapping = MAPPING_C; else
                   if (!strcasecmp(optarg,"C++")) globals.mapping = MAPPING_CXX; else
                     {
                       printf("Unknown mapping: %s\n", optarg);
                       exit(1);
                     }
                   break;    
        case 'p' : if (!strcasecmp(optarg,"ia32")) globals.platform = PLATFORM_IA32; else
                   if (!strcasecmp(optarg,"arm")) globals.platform = PLATFORM_ARM; else
                   if (!strcasecmp(optarg,"generic")) globals.platform = PLATFORM_GENERIC; else
                   if (!strcasecmp(optarg,"dummy")) globals.platform = PLATFORM_DUMMY; else
                   if (!strcasecmp(optarg,"ia64"))
                     {
                       globals.platform = PLATFORM_IA64; 
                       globals.word_size = 8;
                     } else {
                       printf("Unknown platform: %s\n", optarg);
                       exit(1);
                     }
                   break;    
        case 'w' : globals.word_size = strtol(optarg, 0, 10);
                   if (globals.word_size != 32 && globals.word_size != 64)
                     {
                       printf("Word size not supported: %d\n", globals.word_size);
                       exit(1);
                     }
                   globals.word_size /= 8;
                   break;    
        case 'C' : globals.compat_mode = true;
                   break;
        case 2   : globals.cpp_path = optarg;
                   break;
        case 3   : globals.system_prefix = optarg;
                   break;
        case 4   : globals.pre_call_code = optarg;
                   break;
        case 5   : globals.post_call_code = optarg;
                   break;
        default  : help();break;
      }  

  if (argc==optind)
    help();

  dprintln("Debug mode set to %d\n", debug_mode);
    
  strncpy(globals.infile_name, argv[optind], FILENAME_MAX);
  globals.infile_name[FILENAME_MAX-3] = 0;
  globals.outfile_name[FILENAME_MAX-3] = 0;
  globals.header_name[FILENAME_MAX-3] = 0;

  if (!globals.header_name[0])
    strcpy(globals.header_name, getDefaultFilename(FILE_HEADER, globals.output_type));
    
  if (!globals.outfile_name[0])
    strcpy(globals.outfile_name, getDefaultFilename(FILE_CODE, globals.output_type));

  time_t currentTime;
  time(&currentTime);
  globals.time = *localtime(&currentTime);
  
  char *slash = strrchr(globals.infile_name, '/');
  if (slash)
    {
      strncpy(globals.prefix_path, globals.infile_name, (slash - globals.infile_name + 1));
      globals.prefix_path[slash - globals.infile_name + 1] = 0;
    } else strcpy(globals.prefix_path, "");
}

int main(int argc, char *argv[])

{
  parse_args(argc, argv);

  if (globals.interface == INTERFACE_X0)
    {
      switch (globals.platform)
        {
          case PLATFORM_GENERIC : msFactory = new CMSFactoryX();break;
          case PLATFORM_IA32    : msFactory = new CMSFactoryIX();break;
          case PLATFORM_DUMMY   : msFactory = new CMSFactoryDummy();break;
          default               : panic("X0 platforms: generic, ia32, dummy");
        }  
    } else 
  if (globals.interface == INTERFACE_V2)
    {
      switch (globals.platform)
        {
          case PLATFORM_GENERIC : msFactory = new CMSFactory2();break;
          case PLATFORM_IA32    : msFactory = new CMSFactory2I();break;
          case PLATFORM_DUMMY   : msFactory = new CMSFactoryDummy();break;
          default               : panic("V2 platforms: generic, ia32, dummy");
        }  
    } else 
  if (globals.interface == INTERFACE_V4)
    {
      switch (globals.platform)
        {
          case PLATFORM_GENERIC : msFactory = new CMSFactory4();break;
          case PLATFORM_IA32    : msFactory = new CMSFactoryI4();break;
          case PLATFORM_IA64    : msFactory = new CMSFactoryM4();break;
          case PLATFORM_DUMMY   : msFactory = new CMSFactoryDummy();break;
          default               : panic("V4 platforms: generic, ia32, dummy");
        }
    } else panic("Unsupported interface");

  if (globals.mapping == MAPPING_CXX)
    CASTBase::setCXXMode(true);

  aoiFactory = new CAoiFactory();

  aoiRoot = aoiFactory->getRootScope();
  msFactory->initRootScope(aoiRoot);
  
  dprintln("parsing...\n");  
  IDLInputContext *mainFile = new IDLInputContext(globals.infile_name, aoiRoot);
  mainFile->setPreprocIncludes(globals.preproc_includes);
  mainFile->setPreprocDefines(globals.preproc_defines);
  mainFile->setPreprocOptions(globals.preproc_options);
  if (globals.cpp_path)
    mainFile->setCppPath(globals.cpp_path);
  mainFile->Parse();
  
  if (mainFile->hasErrors())
    panic("%s: %i Error(s) and %i Warning(s)", globals.infile_name,
          mainFile->getErrorCount(), mainFile->getWarningCount());

  if (debug_mode & DEBUG_AOI)
    aoiRoot->accept(new CAoiDumpVisitor());  

  dprintln("checking...\n");
  aoiRoot->accept(new CAoiCheckVisitor());

  if ((globals.mapping == MAPPING_C) || (globals.mapping == MAPPING_CXX))
    {
      dprintln("transforming...\n");
      aoiRoot->accept(new CAoiCrossVisitor());

      CBERootScope *beRoot = (CBERootScope*)aoiRoot->peer;
      CBEIDSource *idSource = new CBEIDSource();
      if (!semanticErrors)
        {
          dprintln("assigning IDs...\n");
          beRoot->claimIDs(idSource);
          idSource->commit();
        }  

      if (!semanticErrors)
        {
          dprintln("marshalling...\n");
          beRoot->marshal();
        }
      
      if (semanticErrors>0)
        panic("%s: %i Error(s)", globals.infile_name, semanticErrors);
      
      CASTStatement *headerTree = NULL;
      CASTStatement *codeTree = NULL;
      
      dprintln("generating...\n");
      switch (globals.output_type)
        {
          case OUTPUT_CLIENT :
            headerTree = beRoot->buildClientHeader();
            codeTree = beRoot->buildClientCode();
            dprintln("writing...\n");
            if (headerTree)
              (new CASTFile(globals.header_name, headerTree))->write();
            if (codeTree)
              (new CASTFile(globals.outfile_name, codeTree))->write();
            break;
            
          case OUTPUT_SERVER :
            headerTree = beRoot->buildServerHeader();
            codeTree = beRoot->buildServerCode();
            dprintln("writing...\n");
            if (headerTree)
              (new CASTFile(globals.header_name, headerTree))->write();
            if (codeTree)
              (new CASTFile(globals.outfile_name, codeTree))->write();
            break;
          
          case OUTPUT_TEMPLATE :
            codeTree = beRoot->buildServerTemplate();
            dprintln("writing...\n");
            (new CASTFile(globals.outfile_name, codeTree))->write();
            break;
        }    
    }  

  dprintln("done\n");

  return 0;
}
