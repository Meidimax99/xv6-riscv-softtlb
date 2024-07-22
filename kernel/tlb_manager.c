#include "defs.h"
#include "types.h"
#include "riscv.h"


void tlb_handle_miss(uint64 addr, uint64 satp) {
  w_tp(r_mhartid()); //fix problems with locks based on cpuid()
  //struct proc *p = myproc();
  printf("Enter tlb miss handler!\n");
  //pagetable_t pt =  MAKE_PT(satp);
  //TODO should the tlb miss handler be able to allocate? Probably not -> Page Handlers job
  //pte_t *pte = walk(pt, addr, 0);

  //TODO Verify valid flag
  w_tlbh(addr);
  uint16 prot =  PTE_R | PTE_W | PTE_U | PTE_V;
  w_tlbl(PA2PTE(0x85000000) | prot);
  return;
}
