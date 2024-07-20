#include "defs.h"
#include "types.h"
#include "riscv.h"

void tlb_handle_miss(uint64 addr, uint64 usatp) {
  //uint64 *paddr = kalloc();
  w_tlbh(addr);
  w_tlbl((uint64)addr+(uint64)0x1000);
  return;
}
