#include "defs.h"
#include "types.h"
#include "riscv.h"

#define SATP2ASID(satp) ((satp << 4) >> 48)

uint64 get_mapping(uint64 addr, uint16 asid) {
  if(asid == 0) {
    //Kernel -> direkt mapping
    //TODO special mappings
    return addr;
  } else {
    //mappings for processes
    //TODO trapframe and trampoline mappings
  }
  return 0;
}

void tlb_handle_miss(uint64 addr, uint64 satp) {
  w_tp(r_mhartid()); //fix problems with locks based on cpuid()

  //TODO Locks!
  //printf("tlb_manager: addr=%p satp=%p\n", addr, satp);

  uint64 pa = get_mapping(addr, SATP2ASID(satp));

  w_tlbh(addr);
  uint16 prot =  PTE_R | PTE_W | PTE_U | PTE_V;
  w_tlbl(PA2PTE(pa) | prot);
  return;
}
