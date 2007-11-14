#ifndef __idl4_glue_v4_ia32_runtime_h__
#define __idl4_glue_v4_ia32_runtime_h__

#include IDL4_INC_ARCH(helpers.h)
#include IDL4_INC_API(interface.h)

#define IDL4_INLINE inline

#define IDL4_IPC_ENTRY "__L4_Ipc"
 
extern inline void *MyUTCB(void)

{
  void *result;
  
  __asm__ __volatile__ ( "movl %%gs:(0), %0\n\t" : "=r" (result) );
  
  return result;
}

extern inline void *idl4_get_buffer_addr(unsigned int index)

{
  return ((void**)MyUTCB())[-(18+(index*2))];
}

extern inline int ErrorCode(void)

{
  int result;
  
  __asm__ __volatile__ ( 
    "movl %%gs:(0), %%esi\n\t"
    "movl -36(%%esi), %%esi\n\t" 
    : "=S" (result) 
  );
  
  return result;
}

extern inline void idl4_process_request(L4_ThreadId_t *partner, L4_MsgTag_t *msgtag, idl4_msgbuf_t *msgbuf, long *cnt, void *func)

{
  unsigned dummy;
  
  asm volatile (
                 "push	%%ebp	 		\n\t"
                 "call	*%%ebx	                \n\t"
                 "pop	%%ebp			\n\t"
                 
                 : "=a" (msgtag->raw), "=b" (dummy), "=c" (*cnt), "=d" (partner->raw), "=S" (dummy)
                 : "a" (partner->raw), "b" (func), "d" ((unsigned)msgbuf)
                 : "memory", "cc", "edi"
               );
}

extern inline void idl4_reply_and_wait(L4_ThreadId_t *partner, L4_MsgTag_t *msgtag, idl4_msgbuf_t *msgbuf, long *cnt)

{
  unsigned dummy;

  asm volatile (
  		 "push	%%esi			\n\t"
                 "movl	%%gs:(0), %%edi		\n\t"
                 "addl	$260, %%esi		\n\t"
                 "addl	$4, %%edi		\n\t"
                 "rep	movsl			\n\t"
                 "movl	%%gs:(0), %%edi		\n\t"
                 "movl  %%ebx, %%esi		\n\t"
                 "movl	$0x04000000, %%ecx	\n\t"
                 "movl	$0xFFFFFFFF, %%edx	\n\t"
  		 "push	%%ebp			\n\t"
                 "call " IDL4_IPC_ENTRY "	\n\t"
                 "pop	%%ebp			\n\t"
                 "movl  %%esi, %%edx		\n\t"
		 "movl	%%esi, %%ecx		\n\t"
                 "shr	$6, %%esi		\n\t"
                 "andl	$0x3F, %%esi		\n\t"
                 "addl	%%esi, %%ecx		\n\t"
                 "andl	$0x3F, %%ecx		\n\t"
                 "lea   4(%%edi), %%esi		\n\t"
                 "pop	%%edi			\n\t"
                 "movl	%%edx, (%%edi)		\n\t"
                 "addl  $4, %%edi		\n\t"
                 "rep	movsl			\n\t"
                                                     
                 : "=d" (msgtag->raw), "=a" (partner->raw), "=c" (dummy), "=S" (dummy), "=b" (dummy)
                 : "b" (msgtag->raw), "a" (partner->raw), "c" (*cnt), "S" ((unsigned)msgbuf)
                 : "memory", "cc", "edi"
               );
}               

extern inline void idl4_set_counter(unsigned value)

{
  asm volatile (
    "movl %%gs:(0), %%eax	\n\t"
    "movl %%ebx, -16(%%eax)	\n\t"
    :
    : "b" (value)
    : "eax"
  );
}

extern inline void idl4_set_counter_minimum(unsigned value)

{
  asm volatile (
    "movl %%gs:(0), %%eax	\n\t"
    "orl %%ebx, -16(%%eax)	\n\t"
    :
    : "b" (value)
    : "eax"
  );
}

extern inline void idl4_msgbuf_sync(idl4_msgbuf_t *msgbuf)

{
  unsigned dummy;
  
  asm volatile (
    "movl %%gs:(0), %%eax	\n\t"
    "movl -16(%%eax), %%ecx	\n\t"
    "test %%ecx, %%ecx		\n\t"
    "jz 0f			\n\t"
    "lea -64(%%eax), %%edi	\n\t"
    "std			\n\t"
    "rep movsl			\n\t"
    "cld			\n\t"
    "xor %%ecx, %%ecx		\n\t"
    "movl %%ecx, -16(%%eax)	\n\t"
    "0:				\n\t"
    : "=S" (dummy)
    : "S" ((unsigned)(&msgbuf->rbuf[32]))
    : "eax", "edi", "ecx", "cc"
  );
}

extern inline void idl4_msgbuf_init(idl4_msgbuf_t *msgbuf)

{
  msgbuf->rbuf[32] = 0;
  idl4_set_counter(1);
}

extern inline void idl4_msgbuf_set_rcv_window(idl4_msgbuf_t *msgbuf, L4_Fpage_t wnd)

{
  msgbuf->rbuf[32] = (msgbuf->rbuf[32]&1) + (wnd.raw & 0xFFFFFFF0u);
  idl4_set_counter_minimum(1);
}

extern inline void idl4_msgbuf_add_buffer(idl4_msgbuf_t *msgbuf, void *buf, unsigned len)

{
  int i=32;
 
  while (msgbuf->rbuf[i]&1)
    i -= (i==32) ? 1 : 2;
  
  msgbuf->rbuf[i] |= 1;
  i -= (i==32) ? 1 : 2;
  
  msgbuf->rbuf[i--] = len<<10;
  msgbuf->rbuf[i--] = (unsigned)buf;
  
  idl4_set_counter_minimum((unsigned)(32-i));
}

#endif /* !defined(__idl4_glue_v4_ia32_runtime_h__) */
