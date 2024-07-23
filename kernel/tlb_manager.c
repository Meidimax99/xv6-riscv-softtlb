#include "defs.h"
#include "types.h"
#include "riscv.h"

#define SATP2PA(satp) ((satp << 12) & ~(0xffull << 44))
pte_t *
walk_pt(uint64 satp, uint64 va)
{
  uint64 *pt = (uint64*)SATP2PA(satp);
  if(va >= MAXVA)
    panic("walk");

  for(int level = 2; level > 0; level--) {
    pte_t *pte = &pt[PX(level, va)];
    if(*pte & PTE_V) {
      pt = (uint64*)PTE2PA(*pte);
    } else {
      return 0;
    }
  }
  return &pt[PX(0, va)];
}

void tlb_handle_miss(uint64 addr, uint64 satp) {
  w_tp(r_mhartid()); //fix problems with locks based on cpuid()
  //struct proc *p = myproc();
  //pagetable_t pt =  MAKE_PT(satp);
  //TODO should the tlb miss handler be able to allocate? Probably not -> Page Handlers job
  pte_t *pte = walk_pt(satp, addr);
  if(pte == 0) {
    //TODO no mapping available -> trigger page fault
  }
  //TODO Verify valid flag
  w_tlbh(addr);
  uint16 prot =  PTE_R | PTE_W | PTE_U | PTE_V;
  w_tlbl(*pte | prot);
  return;
}
