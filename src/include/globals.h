#ifndef __globals_h__
#define __globals_h__

#include <stdio.h>
#include <time.h>

#define PARSER_ALLOW_UNBOUNDED
#undef  PARSER_STRICT
#define SERVER_BUFFER_SIZE	8000 	/* Not a power of 2 (cache!) */

#define IDL4_HEADER_REVISION 20060813

/******************************************************************************/

#define OUTPUT_CLIENT 0
#define OUTPUT_SERVER 1
#define OUTPUT_TEMPLATE 2

#define FILE_CODE 0
#define FILE_HEADER 1

#define PLATFORM_IA32       0
#define PLATFORM_IA64       1
#define PLATFORM_ALPHA      2
#define PLATFORM_ARM        3
#define PLATFORM_MIPS64     4
#define PLATFORM_POWERPC    5
#define PLATFORM_GENERIC    6
#define PLATFORM_DUMMY      7

#define INTERFACE_X0 0
#define INTERFACE_V4 1
#define INTERFACE_V2 2

#define MAPPING_C 0
#define MAPPING_CXX 1

#define MAX_DIMENSION 5   	/* array dimensions of a declarator */
#define MAX_CHANNELS 15   	/* number of exceptions per operation + 2 */
#define MAX_OPTIONS 40		/* pass-through options for the preprocessor */

#define WARN_PREALLOC		(1<<0)

#define FLAG_FASTCALL		(1<<0)
#define FLAG_CTYPES		(1<<1)
#define FLAG_NOFRAMEPTR		(1<<2)
#define FLAG_LIPC		(1<<3)
#define FLAG_USEMALLOC		(1<<4)
#define FLAG_NOEMPTYFILES	(1<<5)
#define FLAG_LOOPONLY		(1<<6)
#define FLAG_MODULESONLY	(1<<7)
#define FLAG_GENSUPERTMPL	(1<<8)

typedef struct {
                 char outfile_name[FILENAME_MAX];
                 char header_name[FILENAME_MAX];
                 char infile_name[FILENAME_MAX];
                 char prefix_path[FILENAME_MAX];
                 const char *preproc_includes[MAX_OPTIONS+1];
                 const char *preproc_defines[MAX_OPTIONS+1];
                 const char *preproc_options[MAX_OPTIONS+1];
                 const char *cpp_path;
                 const char *system_prefix;
                 const char *pre_call_code;
                 const char *post_call_code;
                 int output_type;
                 int platform;
                 int interface;
                 int mapping;
                 int flags;
                 int warnings;
                 int word_size;
                 bool compat_mode;
                 struct tm time;
               } global_t;

class CAoiRootScope;
class CAoiFactory;
class CMSFactory;
              
extern const char *idl4_version, *idl4_user, *idl4_gcc, *idl4_build, *idl4_cpp;
extern global_t globals;
extern CAoiRootScope *aoiRoot;
extern CAoiFactory *aoiFactory;
extern CMSFactory *msFactory;

char *getDefaultFilename(int number, int mode);

#define max(a,b) (((a)>(b)) ? (a) : (b))
              
#endif
