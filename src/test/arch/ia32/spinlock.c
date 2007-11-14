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
  *spinlock = 0;
}

