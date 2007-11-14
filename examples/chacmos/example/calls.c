#include "chacmos.h"
#include "stdlib.h"
#include "interfaces/example_server.h"

IDL4_INLINE CORBA_long example_implements_implementation(CORBA_Object _caller, CORBA_long objID, CORBA_long interfaceID, idl4_server_environment *_env)

{
  printf("Task %X checks interface %d...\n",_caller.raw,interfaceID);

  if ((objID==DEFAULT_OBJECT) && (interfaceID==GENERIC_API))
    return EYES;
    
  return ENO;
}

IDL4_PUBLISH_EXAMPLE_IMPLEMENTS(example_implements_implementation);
