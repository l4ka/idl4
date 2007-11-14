#define IDL4_INTERNAL
#include <idl4/test.h>

#define dprintf(a...)

#if defined(CONFIG_ARCH_ALPHA)
#define PAGE_MASK       (0x1FFF)
#define PAGE_BITS 	13
#else
#define PAGE_MASK       (0xFFF)
#define PAGE_BITS 	12
#endif

#define PAGE_SIZE (1<<PAGE_BITS)

#define IDL4_PAGE_MAGIC 0x20121977

static unsigned long sendpage[4][PAGE_SIZE/sizeof(long)] __attribute__ ((aligned (4*PAGE_SIZE)));
static unsigned long recvpage[5][PAGE_SIZE/sizeof(long)] __attribute__ ((aligned (4*PAGE_SIZE)));

long nopf_start = (long)&recvpage[0][0];
long nopf_end   = (long)&recvpage[4][0];

extern "C" idl4_fpage_t idl4_get_sendpage(L4_Word_t randval)

{
  long page_nr = randval&3;
  long send_base = ((randval>>2)&3)*PAGE_SIZE;
  long page_addr = (long)&sendpage[page_nr][0];
  idl4_fpage_t result;

  if (page_addr&0xFFF) 
    panic("Send page %d not page-aligned (at %Xh)", page_nr, page_addr);
    
  result.fpage = page_addr+(PAGE_BITS<<4) + 7;
  result.base = send_base | 8;

dprintf("page=%d fpage=%x base=%x\n", page_nr, result.fpage, result.base);
dprintf("initing at %x\n", (long)&sendpage[0][0]);
  
  sendpage[page_nr][0] = page_nr;
  sendpage[page_nr][1] = IDL4_PAGE_MAGIC;
  sendpage[page_nr][2] = result.fpage;
  sendpage[page_nr][3] = result.base;

dprintf("access completed\n");
  
  return result;
}

extern "C" L4_Fpage_t idl4_get_default_rcv_window()

{
  L4_Fpage_t fp;
  fp.raw = (((unsigned long)&recvpage[0][0])&(~PAGE_MASK))+((PAGE_BITS+2)<<4) + 7;
  return fp;
}

extern "C" void idl4_check_rcvpage(idl4_fpage_t fpage)

{
  long rcv_window = idl4_get_default_rcv_window().raw;
  long rcvwindow_addr = (rcv_window & (~PAGE_MASK));
  long rcvwindow_bits = (rcv_window & 0x2F0)>>4;
  long rcvwindow_mask = ~(((unsigned long)-1)<<rcvwindow_bits);
  long offset = (fpage.base&((unsigned long)-16)) & rcvwindow_mask;
  long effective_addr = offset + rcvwindow_addr;
  L4_Word_t *ptr;
  L4_Fpage_t fp;
  
  if ((fpage.base&0xE)!=8)
    panic("MapItem has incorrect type tag: %X (expected 8)", (fpage.base&0xE));
 
  dprintf("rcvwindow at %Xh, %d bits => mask = %Xh\n", rcvwindow_addr, rcvwindow_bits, rcvwindow_mask);
  dprintf("fpage base = %Xh => offset = %Xh\n", fpage.base, offset);
  dprintf(" => effective addr %Xh\n", effective_addr);
  
  if (effective_addr&PAGE_MASK)
    panic("Effective addr is not page-aligned: %Xh", effective_addr);
  
  if (effective_addr<nopf_start || effective_addr>=nopf_end)
    panic("Received fpage (%Xh => %Xh) not in receive window (%Xh..%Xh)", 
      fpage.fpage, effective_addr, nopf_start, nopf_end);
    
  ptr = (L4_Word_t*)effective_addr;
  if (ptr[1]!=IDL4_PAGE_MAGIC)
    panic("Page magic missing in fpage %Xh at %Xh\n", fpage.fpage, effective_addr);
  if (ptr[2]!=fpage.fpage)
    panic("Wrong page mapped: Expected %Xh at %Xh, got %Xh\n", fpage.fpage, effective_addr, ptr[2]);
  if (ptr[3]!=fpage.base)
    panic("Wrong offset in page %Xh at %Xh: Expected %Xh, got %Xh\n", fpage.fpage, effective_addr, fpage.base, ptr[3]);

  dprintf("validated ok. unmapping... ");

  fp.raw = effective_addr | (PAGE_BITS<<4) | 7;

  L4_Flush(fp);
  
  dprintf("done\n");
}
