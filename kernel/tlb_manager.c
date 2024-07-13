#include "defs.h"
#include "types.h"


void tlb_handle_miss(uint64 addr) {
  printf("Hello from the tlb manager!\n");
  printf("Faulting address: %p\n",addr);
  return;
}
