#include "stdlib.h"
#include "interfaces/booter_server.h"

void *booter_vtable_1[BOOTER_DEFAULT_VTABLE_SIZE] = BOOTER_DEFAULT_VTABLE_1;
void *booter_vtable_2[BOOTER_DEFAULT_VTABLE_SIZE] = BOOTER_DEFAULT_VTABLE_2;
void *booter_vtable_4[BOOTER_DEFAULT_VTABLE_SIZE] = BOOTER_DEFAULT_VTABLE_4;
void *booter_vtable_5[BOOTER_DEFAULT_VTABLE_SIZE] = BOOTER_DEFAULT_VTABLE_5;
void *booter_vtable_discard[BOOTER_DEFAULT_VTABLE_SIZE] = BOOTER_DEFAULT_VTABLE_DISCARD;
void **booter_itable[8] = { booter_vtable_discard, booter_vtable_1, booter_vtable_2, booter_vtable_discard, booter_vtable_4, booter_vtable_5, booter_vtable_discard, booter_vtable_discard };

void booter_server()

{
  unsigned int msgdope, dummy, fnr, reply, w0, w1, w2;
  l4_threadid_t partner;
  struct {
    unsigned int stack[768];
    l4_fpage_t rcv_window;
    unsigned int size_dope;
    unsigned int send_dope;
    unsigned int message[BOOTER_MSGBUF_SIZE];
    idl4_strdope_t str[BOOTER_STRBUF_SIZE];
  } buffer;

  for (w0 = 0;w0 < BOOTER_STRBUF_SIZE;w0++)
    {
      buffer.str[w0].rcv_addr = malloc(8000);
      buffer.str[w0].rcv_size = 8000;
    }

  while (1)
    {
      buffer.size_dope = BOOTER_RCV_DOPE;
      buffer.rcv_window = idl4_nilpage;
      partner = idl4_nilthread;
      reply = idl4_nil;
      w0 = w1 = w2 = 0;

      while (1)
        {
          idl4_reply_and_wait(reply, buffer, partner, msgdope, fnr, w0, w1, w2, dummy);

          if (msgdope & 0xF0)
            break;

          idl4_process_request(booter_itable[(fnr >> IDL4_FID_BITS) & BOOTER_IID_MASK], fnr & BOOTER_FID_MASK, reply, buffer, partner, w0, w1, w2, dummy);
        }

      enter_kdebug("message error");
    }
}

void booter_discard()

{
  enter_kdebug("discarding request for booter");
}
