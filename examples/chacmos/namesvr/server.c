#include "stdlib.h"
#include "interfaces/namesvr_server.h"

void *namesvr_vtable_1[NAMESVR_DEFAULT_VTABLE_SIZE] = NAMESVR_DEFAULT_VTABLE_1;
void *namesvr_vtable_2[NAMESVR_DEFAULT_VTABLE_SIZE] = NAMESVR_DEFAULT_VTABLE_2;
void *namesvr_vtable_6[NAMESVR_DEFAULT_VTABLE_SIZE] = NAMESVR_DEFAULT_VTABLE_6;
void *namesvr_vtable_7[NAMESVR_DEFAULT_VTABLE_SIZE] = NAMESVR_DEFAULT_VTABLE_7;
void *namesvr_vtable_discard[NAMESVR_DEFAULT_VTABLE_SIZE] = NAMESVR_DEFAULT_VTABLE_DISCARD;
void **namesvr_itable[8] = { namesvr_vtable_discard, namesvr_vtable_1, namesvr_vtable_2, namesvr_vtable_discard, namesvr_vtable_discard, namesvr_vtable_discard, namesvr_vtable_6, namesvr_vtable_7 };

void namesvr_server()

{
  unsigned int msgdope, dummy, fnr, reply, w0, w1, w2;
  l4_threadid_t partner;
  struct {
    unsigned int stack[768];
    l4_fpage_t rcv_window;
    unsigned int size_dope;
    unsigned int send_dope;
    unsigned int message[NAMESVR_MSGBUF_SIZE];
    idl4_strdope_t str[NAMESVR_STRBUF_SIZE];
  } buffer;

  for (w0 = 0;w0 < NAMESVR_STRBUF_SIZE;w0++)
    {
      buffer.str[w0].rcv_addr = malloc(8000);
      buffer.str[w0].rcv_size = 8000;
    }

  while (1)
    {
      buffer.size_dope = NAMESVR_RCV_DOPE;
      buffer.rcv_window = idl4_nilpage;
      partner = idl4_nilthread;
      reply = idl4_nil;
      w0 = w1 = w2 = 0;

      while (1)
        {
          idl4_reply_and_wait(reply, buffer, partner, msgdope, fnr, w0, w1, w2, dummy);

          if (msgdope & 0xF0)
            break;

          idl4_process_request(namesvr_itable[(fnr >> IDL4_FID_BITS) & NAMESVR_IID_MASK], fnr & NAMESVR_FID_MASK, reply, buffer, partner, w0, w1, w2, dummy);
        }

      enter_kdebug("message error");
    }
}

void namesvr_discard()

{
}
