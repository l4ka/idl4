#define IDL4_INTERNAL
#include <idl4/test.h>

#define dprintf(a...)

#define IDL4_PAGE_MAGIC 0x20121977

static unsigned sendpage[4][1024] __attribute__ ((aligned (16384)));
static unsigned recvpage[5][1024] __attribute__ ((aligned (16384)));

unsigned nopf_start = (unsigned)&recvpage[0][0];
unsigned nopf_end   = (unsigned)&recvpage[4][0];

idl4_fpage_t idl4_get_sendpage(unsigned randval)

{
  int page_nr = randval&3;
  int send_base = ((randval>>2)&3)*0x1000;
  unsigned page_addr = (unsigned)&sendpage[page_nr][0];
  idl4_fpage_t result;
  
  if (page_addr&0xFFF) 
    panic("Send page %d not page-aligned (at %Xh)", page_nr, page_addr);
    
  result.fpage = page_addr+(12<<2);
  result.base = send_base;
  
  sendpage[page_nr][0] = page_nr;
  sendpage[page_nr][1] = IDL4_PAGE_MAGIC;
  sendpage[page_nr][2] = result.fpage;
  sendpage[page_nr][3] = result.base;
  
  return result;
}

l4_fpage_t idl4_get_default_rcv_window()

{
  l4_fpage_t fp;
  fp.raw = (((unsigned)&recvpage[0][0])&0xFFFFF000)+((12+2)<<2);
  return fp;
}

void idl4_check_rcvpage(idl4_fpage_t fpage)

{
  unsigned rcv_window = idl4_get_default_rcv_window().raw;
  unsigned rcvwindow_addr = (rcv_window & 0xFFFFF000);
  unsigned rcvwindow_bits = (rcv_window & 0xFC)>>2;
  unsigned rcvwindow_mask = ~(0xFFFFFFFF<<rcvwindow_bits);
  unsigned offset = fpage.base & rcvwindow_mask;
  unsigned effective_addr = offset + rcvwindow_addr;
  unsigned *ptr;
  l4_fpage_t fp;
 
  dprintf("rcvwindow at %Xh, %d bits => mask = %Xh\n", rcvwindow_addr, rcvwindow_bits, rcvwindow_mask);
  dprintf("fpage base = %Xh => offset = %Xh\n", fpage.base, offset);
  dprintf(" => effective addr %Xh\n", effective_addr);
  
  if (effective_addr&0xFFF)
    panic("Effective addr is not page-aligned: %Xh", effective_addr);
  
  if (effective_addr<nopf_start || effective_addr>=nopf_end)
    panic("Received fpage (%Xh => %Xh) not in receive window (%Xh..%Xh)", 
      fpage.fpage, effective_addr, nopf_start, nopf_end);
    
  ptr = (unsigned*)effective_addr;
  if (ptr[1]!=IDL4_PAGE_MAGIC)
    panic("Page magic missing in fpage %Xh at %Xh\n", fpage.fpage, effective_addr);
  if (ptr[2]!=fpage.fpage)
    panic("Wrong page mapped: Expected %Xh at %Xh, got %Xh\n", fpage.fpage, effective_addr, ptr[2]);
  if (ptr[3]!=fpage.base)
    panic("Wrong offset in page %Xh at %Xh: Expected %Xh, got %Xh\n", fpage.fpage, effective_addr, fpage.base, ptr[3]);

  dprintf("validated ok. unmapping... ");

  fp.raw = effective_addr + (12<<2);
  l4_fpage_unmap(fp, 0x80000002);
  
  dprintf("done\n");
}
