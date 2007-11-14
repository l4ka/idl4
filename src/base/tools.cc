#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "base.h"

char *getDefaultFilename(int number, int mode)

{
  const char *modelName = (globals.outfile_name[0]) ? globals.outfile_name : globals.infile_name;
  static char infile[FILENAME_MAX];
  const char *std_suffix[3] = { "_client", "_server", "_template" };
  int x,y;
  
  assert(number==FILE_CODE || number==FILE_HEADER);

  for (x=strlen(modelName)-1;(x>0) && (!strchr("./",modelName[x]));x--);
  if (modelName[x]!='.') x = strlen(modelName);
  if (x>(FILENAME_MAX - 8)) 
    x = FILENAME_MAX - 8;

  for (y=x;(y>0) && modelName[y]!='/';y--);
  if (modelName[y]=='/') y++;

  if (x>y)
    strncpy(infile, &modelName[y], x - y);
  infile[x - y] = 0;
  
  strcat(infile, std_suffix[mode]);
  switch (number)
    {
      case FILE_CODE   : strcat(infile, ".c");
                         break;
      case FILE_HEADER : strcat(infile, ".h");
                         break;
    }

  return mprintf(infile);
}

const char *getBuiltinMacro(int nr)

{
  if (nr!=0)
    return NULL;
    
  static char version_string[20];
  strcpy(version_string, idl4_version);
  
  char *c = strtok(version_string, ".");
  int major = strtol(c, NULL, 10);
  c = strtok(NULL, ".");
  int minor = strtol(c, NULL, 10);
  c = strtok(NULL, ".");
  int subver = strtol(c, NULL, 10);
  
  sprintf(version_string, "__IDL4__=%d%2d%2d", major, minor, subver);
  for (c=version_string; *c; c++)
    if (*c==' ') 
      *c = '0';

  return version_string;
}
