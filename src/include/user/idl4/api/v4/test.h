#ifndef __idl4_api_v4_test_h__
#define __idl4_api_v4_test_h__

#include <l4/thread.h>
#include <l4/kdebug.h>
#include <l4/ipc.h>

#ifdef __cplusplus
extern "C"

{
#endif
  void test(L4_ThreadId_t server); 
  void idl4_thread_init(L4_ThreadId_t thread, L4_Word_t eip);
  idl4_fpage_t idl4_get_sendpage(L4_Word_t randval);
  void idl4_check_rcvpage(idl4_fpage_t fpage);
#ifdef __cplusplus
  L4_Fpage_t idl4_get_default_rcv_window();
#else
  L4_Fpage_t idl4_get_default_rcv_window(void);
#endif  
#ifdef __cplusplus
}
#endif

inline static void idl4_yield_to(L4_ThreadId_t target)

{
  L4_ThreadSwitch(target);
} 

inline static L4_ThreadId_t idl4_myself(void)

{
  return L4_MyGlobalId();
}

inline static L4_ThreadId_t idl4_get_random_threadid(void)

{
  L4_ThreadId_t temp;
  temp.raw = random();
  return temp;
}

#endif /* !defined(__idl4_api_v4_test_h__) */
