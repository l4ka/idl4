#define IDL4_INTERNAL
#include <idl4/corba.h>
#include <idl4/test.h>

CORBA_wchar *CORBA_wstring_alloc(unsigned size) 

{ 
  return (CORBA_wchar*)malloc(((unsigned)(size)+1)*sizeof(short)); 
}

CORBA_char *CORBA_string_alloc(unsigned size) 

{ 
  return (char*)malloc((unsigned)(size)+1); 
}

CORBA_float *CORBA_float_alloc(unsigned size) 

{ 
  return (float*)malloc(((unsigned)(size)+1)*sizeof(float)); 
}

CORBA_double *CORBA_double_alloc(unsigned size) 

{ 
  return (double*)malloc(((unsigned)(size)+1)*sizeof(double)); 
}

CORBA_long_double *CORBA_long_double_alloc(unsigned size) 

{ 
  return (long double*)malloc(((unsigned)(size)+1)*sizeof(long double)); 
}

void *CORBA_alloc(unsigned size) 
 
{ 
  return malloc((unsigned)(size)+1); 
}

void CORBA_free(void *what) 

{ 
  free(what);
}
