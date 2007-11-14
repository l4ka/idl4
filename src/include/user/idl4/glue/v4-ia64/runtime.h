#ifndef __idl4_glue_v4_ia64_runtime_h__
#define __idl4_glue_v4_ia64_runtime_h__

#include IDL4_INC_ARCH(helpers.h)
#include IDL4_INC_API(interface.h)
#include <idl4/glue/v4-generic/msgbuf.h>
#include <l4/ipc.h>

#define IDL4_INLINE inline

static inline void idl4_process_request(L4_ThreadId_t *partner, L4_MsgTag_t *msgtag, idl4_msgbuf_t *msgbuf, long *cnt, int (*func)(L4_ThreadId_t, idl4_msgbuf_t*))

{
  *cnt = func(*partner, msgbuf);
  if (*cnt<0)
    {
      *partner = L4_nilthread;
      *cnt = 0;
    }
}

static inline void idl4_reply_and_wait(L4_ThreadId_t *partner, L4_MsgTag_t *msgtag, idl4_msgbuf_t *msgbuf, long *cnt)

{
  L4_MsgTag_t _result;
  
  L4_MsgLoad((L4_Msg_t*)&msgbuf->obuf);
  _result = L4_ReplyWait(*partner, partner);
  L4_MsgStore(_result, (L4_Msg_t*)msgbuf);
  *msgtag = _result;
}               

static inline void *idl4_get_buffer_addr(unsigned int idx)

{
  L4_Word_t tmp;
  L4_StoreBR(2 + idx*2, &tmp);
  return (void*)tmp;
}

#endif /* !defined(__idl4_glue_v4_ia64_runtime_h__) */
