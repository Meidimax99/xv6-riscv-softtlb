#include "defs.h"
#include "types.h"
#include "riscv.h"


void tlb_handle_miss(uint64 addr, uint64 satp) {
  printf("Enter tlb miss handler!\n");
  pagetable_t pt =  MAKE_PT(satp);
  //TODO should the tlb miss handler be able to allocate? Probably not -> Page Handlers job
  pte_t *pte = walk(pt, addr, 0);

  //TODO Verify valid flag
  w_tlbh(addr);
  w_tlbl(*pte);
  return;
}
