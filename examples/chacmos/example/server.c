#include "stdlib.h"
#include "interfaces/example_server.h"

void *example_vtable[EXAMPLE_DEFAULT_VTABLE_SIZE] = EXAMPLE_DEFAULT_VTABLE;

void example_server()

{
  unsigned int msgdope, dummy, fnr, reply, w0, w1, w2;
  l4_threadid_t partner;
  struct {
    unsigned int stack[768];
    l4_fpage_t rcv_window;
    unsigned int size_dope;
    unsigned int send_dope;
    unsigned int message[EXAMPLE_MSGBUF_SIZE];
    idl4_strdope_t str[EXAMPLE_STRBUF_SIZE];
  } buffer;

  while (1)
    {
      buffer.size_dope = EXAMPLE_RCV_DOPE;
      buffer.rcv_window = idl4_nilpage;
      partner = idl4_nilthread;
      reply = idl4_nil;
      w0 = w1 = w2 = 0;

      while (1)
        {
          idl4_reply_and_wait(reply, buffer, partner, msgdope, fnr, w0, w1, w2, dummy);

          if (msgdope & 0xF0)
            break;

          idl4_process_request(example_vtable, fnr & EXAMPLE_FID_MASK, reply, buffer, partner, w0, w1, w2, dummy);
        }

      enter_kdebug("message error");
    }
}

void example_discard()

{
}
