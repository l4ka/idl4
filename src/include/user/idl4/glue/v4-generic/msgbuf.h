#ifndef __idl4_glue_v4_generic_msgbuf_h__
#define __idl4_glue_v4_generic_msgbuf_h__

static inline void idl4_set_counter(L4_Word_t value)

{
  __L4_TCR_Set_ThreadWord(0, value);
}

static inline void idl4_msgbuf_sync(idl4_msgbuf_t *msgbuf)

{
  unsigned restoreCount = __L4_TCR_ThreadWord(0), i;
  
  if (restoreCount)
    {
      for (i=0;i<restoreCount;i++)
        L4_LoadBR(i, msgbuf->rbuf[32-i]);
      idl4_set_counter(0);
    }
}

static inline void idl4_set_counter_minimum(unsigned value)

{
  unsigned oldValue = __L4_TCR_ThreadWord(0);
  
  if (oldValue<value)
    __L4_TCR_Set_ThreadWord(0, value);
}

static inline void idl4_msgbuf_init(idl4_msgbuf_t *msgbuf)

{
  msgbuf->rbuf[32] = 0;
  idl4_set_counter(1);
}

static inline void idl4_msgbuf_set_rcv_window(idl4_msgbuf_t *msgbuf, L4_Fpage_t wnd)

{
  msgbuf->rbuf[32] = (msgbuf->rbuf[32]&1) + (wnd.raw & 0xFFFFFFF0u);
  idl4_set_counter_minimum(1);
}

static inline void idl4_msgbuf_add_buffer(idl4_msgbuf_t *msgbuf, void *buf, unsigned len)

{
  int i=32;
 
  while (msgbuf->rbuf[i]&1)
    i -= (i==32) ? 1 : 2;
  
  msgbuf->rbuf[i] |= 1;
  i -= (i==32) ? 1 : 2;
  
  msgbuf->rbuf[i--] = len<<10;
  msgbuf->rbuf[i--] = (unsigned long)buf;
  
  idl4_set_counter_minimum((unsigned)(32-i));
}

#endif /* !defined(__idl4_glue_v4_generic_msgbuf_h__) */
