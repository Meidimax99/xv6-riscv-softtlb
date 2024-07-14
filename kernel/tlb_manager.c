#include "defs.h"
#include "types.h"

//TODO addr not the proper argument
void tlb_handle_miss(uint64 addr) {
  //Printing wont work in m mode
  //printf("Hello from the tlb manager!\n");
  //printf("Faulting address: %p\n",addr);

  //Mess a little with registers
  int a = 54;
  a++;
  int b = 50;
  int c = a * b;
  (void)c;
  return;
}
