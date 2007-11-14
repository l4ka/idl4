#ifndef __idl4_api_v2_test_h__
#define __idl4_api_v2_test_h__

#include "l4/sys/syscalls.h"

#ifdef __cplusplus
extern "C"

{
#endif
  void test(l4_threadid_t server); 
  void idl4_thread_init(l4_threadid_t thread, unsigned int eip);
  idl4_fpage_t idl4_get_sendpage(unsigned randval);
  void idl4_check_rcvpage(idl4_fpage_t fpage);
#ifdef __cplusplus
  l4_fpage_t idl4_get_default_rcv_window();
#else
  l4_fpage_t idl4_get_default_rcv_window(void);
#endif
#ifdef __cplusplus
}
#endif

__inline__ static void idl4_yield_to(l4_threadid_t target)

{
  l4_thread_switch(target);
} 

inline extern l4_threadid_t idl4_myself(void)

{
  return l4_myself();
}

inline extern l4_threadid_t idl4_get_random_threadid(void)

{
  l4_threadid_t temp;
  temp.lh.low = random();
  temp.lh.high = random();
  return temp;
}

#endif /* defined(__idl4_api_v2_test_h__) */
