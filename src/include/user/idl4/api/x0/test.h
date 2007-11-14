#ifndef __idl4_api_x0_test_h__
#define __idl4_api_x0_test_h__

#ifdef __cplusplus
extern "C"

{
#endif
  void test(l4_threadid_t server); 
  void idl4_thread_init(l4_threadid_t thread, unsigned int eip);
  void idl4_start_thread(l4_threadid_t target, l4_threadid_t pager, char *esp, void (*eip)(void));
  idl4_fpage_t idl4_get_sendpage(unsigned randval);
  void idl4_check_rcvpage(idl4_fpage_t fpage);
  l4_fpage_t idl4_get_default_rcv_window(void);
#ifdef __cplusplus
}
#endif

inline static void idl4_yield_to(l4_threadid_t target)

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
  temp.raw = random();
  return temp;
}

#endif /* defined(__idl4_api_x0_test_h__) */
