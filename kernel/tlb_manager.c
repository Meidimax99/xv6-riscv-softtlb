#include "defs.h"
#include "types.h"
#include "riscv.h"

void tlb_handle_miss(uint64 addr, uint64 usatp) {
  uint64 mcause = r_mcause();
  (void)mcause;
  w_tlbh(addr);
  w_tlbl(addr);
  
  return;
}
