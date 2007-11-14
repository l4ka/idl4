#include "stdlib.h"
#include "interfaces/filesvr_server.h"

void * filesvr_vtable_1[FILESVR_DEFAULT_VTABLE_SIZE] = FILESVR_DEFAULT_VTABLE_1;
void * filesvr_vtable_6[FILESVR_DEFAULT_VTABLE_SIZE] = FILESVR_DEFAULT_VTABLE_6;
void * filesvr_vtable_7[FILESVR_DEFAULT_VTABLE_SIZE] = FILESVR_DEFAULT_VTABLE_7;
void * filesvr_vtable_8[FILESVR_DEFAULT_VTABLE_SIZE] = FILESVR_DEFAULT_VTABLE_8;
void * filesvr_vtable_discard[FILESVR_DEFAULT_VTABLE_SIZE] = FILESVR_DEFAULT_VTABLE_DISCARD;
void **filesvr_itable[16] = { filesvr_vtable_discard, filesvr_vtable_1, filesvr_vtable_discard, filesvr_vtable_discard, filesvr_vtable_discard, filesvr_vtable_discard, filesvr_vtable_6, filesvr_vtable_7, filesvr_vtable_8, filesvr_vtable_discard, filesvr_vtable_discard, filesvr_vtable_discard, filesvr_vtable_discard, filesvr_vtable_discard, filesvr_vtable_discard, filesvr_vtable_discard };

void filesvr_server()

{
  unsigned int msgdope, dummy, fnr, reply, w0, w1, w2;
  l4_threadid_t partner;
  struct {
    unsigned int stack[768];
    l4_fpage_t rcv_window;
    unsigned int size_dope;
    unsigned int send_dope;
    unsigned int message[FILESVR_MSGBUF_SIZE];
    idl4_strdope_t str[FILESVR_STRBUF_SIZE];
  } buffer;

  for (w0 = 0;w0 < FILESVR_STRBUF_SIZE;w0++)
    {
      buffer.str[w0].rcv_addr = malloc(8000);
      buffer.str[w0].rcv_size = 8000;
    }

  while (1)
    {
      buffer.size_dope = FILESVR_RCV_DOPE;
      buffer.rcv_window = idl4_nilpage;
      partner = idl4_nilthread;
      reply = idl4_nil;
      w0 = w1 = w2 = 0;

      while (1)
        {
          idl4_reply_and_wait(reply, buffer, partner, msgdope, fnr, w0, w1, w2, dummy);

          if (msgdope & 0xF0)
            break;

          idl4_process_request(filesvr_itable[(fnr >> IDL4_FID_BITS) & FILESVR_IID_MASK], fnr & FILESVR_FID_MASK, reply, buffer, partner, w0, w1, w2, dummy);
        }

      enter_kdebug("message error");
    }
}

void filesvr_discard()

{
}
