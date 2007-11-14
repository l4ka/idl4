#include <stdlib.h>

#ifdef DEBUG
#define dprintf(a...) printf(a)
#else
#define dprintf(a...)
#endif

#define MAX_AREA 	40
#define MAX_ALLOC	65536

void panic(const char *fmt, ...);

struct areaDescriptor {
  int start;
  int length;
  char isServer;
};

static int watermark = 0;
static int maxDescriptor = 0;
static volatile int spinlock = 0;
static struct areaDescriptor areaDescriptor[MAX_AREA];
static char areaBuffer[MAX_ALLOC];

void take(volatile int *spinlock)

{
  int old;
  
  do {
       asm volatile ( 
                      "xchg %%eax, %2\n\t"
                      : "=a" (old)
                      : "a" (1), "m" (*spinlock)
                    );  
     } while (old!=0);
}

void release(volatile int *spinlock)

{
  if (!*spinlock)
    panic("release: spinlock not taken");
    
  *spinlock = 0;
}

void *malloc(unsigned int size)

{
  int nDesc;
  
  take(&spinlock);
  
  for (nDesc=0;(nDesc<maxDescriptor) && areaDescriptor[nDesc].length;nDesc++);
  if (nDesc==maxDescriptor)
    {
      dprintf("Allocated descriptor %d\n", nDesc);
      maxDescriptor++;
      if (maxDescriptor==MAX_AREA)
        panic("malloc: maximum allocation count exceeded");
    }
  
  while (size%16) size++;
    
  if ((size+watermark)>MAX_ALLOC)
    panic("malloc: allocation size exceeded, cannot allocate %d bytes", size);
    
  areaDescriptor[nDesc].start = watermark;
  areaDescriptor[nDesc].length = size;
  areaDescriptor[nDesc].isServer = 0;

  dprintf("Allocated chunk of %d bytes at %X (descriptor %d)\n", size, (int)&areaBuffer[areaDescriptor[nDesc].start], nDesc);
  
  watermark += size;
  dprintf("Watermark is now %d bytes\n", watermark);
  
  release(&spinlock);
  
  return &areaBuffer[areaDescriptor[nDesc].start];
}

void showalloc(void)

{
  int i;
  
  for (i=0;i<maxDescriptor;i++)
    printf("[%d] start=%X [@%X], length=%X, isServer=%X\n",  i,
      areaDescriptor[i].start,  (int)&areaBuffer[areaDescriptor[i].start],
      areaDescriptor[i].length,
      areaDescriptor[i].isServer);
}

void free(void *ptr)

{
  int nDesc;
  
  take(&spinlock);
  for (nDesc=0;(nDesc<maxDescriptor) && (ptr!=&areaBuffer[areaDescriptor[nDesc].start]);nDesc++);
  if (nDesc == maxDescriptor)
    panic("mfree: cannot find chunk at %Xh", (int)ptr);

  dprintf("Released chunk of %d bytes at %X\n", areaDescriptor[nDesc].length, (int)&areaBuffer[areaDescriptor[nDesc].start]);
    
  areaDescriptor[nDesc].start = 0;
  areaDescriptor[nDesc].length = 0;
  
  watermark = 0;
  for (nDesc=0;nDesc<maxDescriptor;nDesc++)
    if (areaDescriptor[nDesc].length)
      if ((areaDescriptor[nDesc].start + areaDescriptor[nDesc].length) > watermark)
        watermark = (areaDescriptor[nDesc].start + areaDescriptor[nDesc].length);

  dprintf("Watermark is now %d bytes\n", watermark);
  release(&spinlock);
}
