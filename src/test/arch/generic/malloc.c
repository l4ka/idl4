#ifdef DEBUG
int printf(const char *fmt, ...);
#define dprintf(a...) printf(a)
#else
#define dprintf(a...)
#endif

#define MAX_AREA 	40
#define MAX_ALLOC	1048576

void panic(const char *fmt, ...);
void take(volatile int *spinlock);
void release(volatile int *spinlock);

struct areaDescriptor {
  int start;
  int length;
  char isServer;
  char inUse;
};

static int watermark = 0;
static int maxDescriptor = 0;
static struct areaDescriptor areaDescriptor[MAX_AREA];
static char areaBuffer[MAX_ALLOC];

static volatile int spinlock = 0;

void *malloc(unsigned int size)

{
  int nDesc;

  take(&spinlock);
  
  for (nDesc=0;(nDesc<maxDescriptor) && areaDescriptor[nDesc].inUse;nDesc++);
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
  areaDescriptor[nDesc].inUse = 1;

  dprintf("Allocated chunk of %d bytes at %X (descriptor %d)\n", size, (int)&areaBuffer[areaDescriptor[nDesc].start], nDesc);
  
  watermark += size;
  dprintf("Watermark is now %d bytes\n", watermark);
  
  release(&spinlock);
  
  return &areaBuffer[areaDescriptor[nDesc].start];
}

void *smalloc(unsigned int size)

{
  void *result = malloc(size);
  int nDesc;
  
  take(&spinlock);
  for (nDesc=0;(nDesc<maxDescriptor) && (result!=&areaBuffer[areaDescriptor[nDesc].start]);nDesc++);
  if (nDesc == maxDescriptor)
    panic("smalloc: allocation failed");
    
  areaDescriptor[nDesc].isServer = 1;
  release(&spinlock);
  
  return result;
}

void *firstalloc(void)

{
  int nDesc;
  void *result = (void*)0;

  take(&spinlock);
  for (nDesc=0;nDesc<maxDescriptor;nDesc++)
    if (areaDescriptor[nDesc].inUse && !areaDescriptor[nDesc].isServer)
      result = &areaBuffer[areaDescriptor[nDesc].start];

  release(&spinlock);
      
  return result;
}

void showalloc(void)

{
  int i;
  
  for (i=0;i<maxDescriptor;i++)
    dprintf("[%d] start=%X [@%X], length=%X, isServer=%X, inUse=%X\n",  i,
      areaDescriptor[i].start,  (int)&areaBuffer[areaDescriptor[i].start],
      areaDescriptor[i].length, areaDescriptor[i].isServer,
      areaDescriptor[i].inUse);
}

void sfree(void)

{
  if (firstalloc())
    panic("sfree: chunk at %Xh was not freed", firstalloc());
    
  take(&spinlock);
  maxDescriptor = 0;
  watermark = 0;
  release(&spinlock);
}

void free(void *ptr)

{
  int nDesc;
  
  take(&spinlock);
  for (nDesc=0;(nDesc<maxDescriptor) && (ptr!=&areaBuffer[areaDescriptor[nDesc].start]);nDesc++);
  if (nDesc == maxDescriptor)
    panic("mfree: cannot find chunk at %ph", ptr);

  dprintf("Released chunk of %d bytes at %X\n", areaDescriptor[nDesc].length, (int)&areaBuffer[areaDescriptor[nDesc].start]);
    
  areaDescriptor[nDesc].start = 0;
  areaDescriptor[nDesc].length = 0;
  areaDescriptor[nDesc].inUse = 0;
  
  watermark = 0;
  for (nDesc=0;nDesc<maxDescriptor;nDesc++)
    if (areaDescriptor[nDesc].length)
      if ((areaDescriptor[nDesc].start + areaDescriptor[nDesc].length) > watermark)
        watermark = (areaDescriptor[nDesc].start + areaDescriptor[nDesc].length);

  dprintf("Watermark is now %d bytes\n", watermark);
  release(&spinlock);
}
