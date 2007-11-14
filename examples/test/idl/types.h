#ifndef __types_h__
#define __types_h__

typedef int ext_int_t;

struct ext_struct_t

{
  char a;
  int b;
  short c;
};

typedef struct {
                 short a;
                 int b,c,d;
               } ext_alias_t;
  
#endif /* defined(__types_h__) */
