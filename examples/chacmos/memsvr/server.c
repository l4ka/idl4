#include "stdlib.h"
#include "interfaces/memsvr_server.h"

void *memsvr_vtable_1[MEMSVR_DEFAULT_VTABLE_SIZE] = MEMSVR_DEFAULT_VTABLE_1;
void *memsvr_vtable_2[MEMSVR_DEFAULT_VTABLE_SIZE] = MEMSVR_DEFAULT_VTABLE_2;
void *memsvr_vtable_3[MEMSVR_DEFAULT_VTABLE_SIZE] = MEMSVR_DEFAULT_VTABLE_3;
void *memsvr_vtable_discard[MEMSVR_DEFAULT_VTABLE_SIZE] = MEMSVR_DEFAULT_VTABLE_DISCARD;
void **memsvr_itable[4] = { memsvr_vtable_discard, memsvr_vtable_1, memsvr_vtable_2, memsvr_vtable_3 };

void memsvr_server()

{
  unsigned int msgdope, dummy, fnr, reply, w0, w1, w2;
  l4_threadid_t partner;
  struct {
    unsigned int stack[768];
    l4_fpage_t rcv_window;
    unsigned int size_dope;
    unsigned int send_dope;
    unsigned int message[MEMSVR_MSGBUF_SIZE];
    idl4_strdope_t str[MEMSVR_STRBUF_SIZE];
  } buffer;

  for (w0 = 0;w0 < MEMSVR_STRBUF_SIZE;w0++)
    {
      buffer.str[w0].rcv_addr = malloc(8000);
      buffer.str[w0].rcv_size = 8000;
    }

  while (1)
    {
      buffer.size_dope = MEMSVR_RCV_DOPE;
      buffer.rcv_window = idl4_nilpage;
      partner = idl4_nilthread;
      reply = idl4_nil;
      w0 = w1 = w2 = 0;

      while (1)
        {
          idl4_reply_and_wait(reply, buffer, partner, msgdope, fnr, w0, w1, w2, dummy);

          if (msgdope & 0xF0)
            break;

          idl4_process_request(memsvr_itable[(fnr >> IDL4_FID_BITS) & MEMSVR_IID_MASK], fnr & MEMSVR_FID_MASK, reply, buffer, partner, w0, w1, w2, dummy);
        }

      enter_kdebug("message error");
    }
}

void memsvr_discard()

{
}

