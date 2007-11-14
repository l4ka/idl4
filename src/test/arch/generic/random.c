static unsigned int __seed = 0xc01ad05e;

unsigned int random(void)

{
  __seed = 0x015a4e35L*__seed + 1;
  return __seed;
}

void *randomPtr(void)

{
  long int x = random();
  return (void*)x;
}
