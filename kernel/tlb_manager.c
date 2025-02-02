#include "defs.h"
#include "param.h"
#include "types.h"
#include "riscv.h"
#include "memlayout.h"

#define SATP2ASID(satp) ((satp << 4) >> 48)

extern char trampoline[];
paddr get_mapping(vaddr addr, uint16 asid) {
  //Special case for testing deceased meat
   
  if(PGROUNDDOWN(addr) == 0x84fff000) {
    return 0x85000000;
  }
  //special case for trampoline, which is in every address space at the same address
  // (except for the physical one!)
  if(PGROUNDDOWN(addr) == TRAMPOLINE) {
    return (uint64)trampoline;
  }
  if(asid == 0) {
    //Kernel -> direkt mapping
    //TODO special mappings
    return addr;
  } else {
    //mappings for processes
    //TODO trapframe and trampoline mappings
    if(PGROUNDDOWN(addr) == TRAPFRAME) {
      return TRAPFRAME_FROM_ASID(asid);
    }

    //Base case 
    uint64 vpage = PGROUNDDOWN(addr);
    if(vpage >= MAX_AS_MEM - (0x1000 * 4)) {
      //Error, address out of range
      panic("tlb_manager: address to high!");
    }
    return AS_START(asid) + vpage;
  }
  return 0;
}

void tlb_handle_miss(vaddr addr, uint64 satp) {
  w_tp(r_mhartid()); //fix problems with locks based on cpuid()

  //TODO Locks!
  //TODO buffering printer, only prints completed lines
  //uint16 mmuid = addr & 0xfff;
  vaddr addr_no_mmuid = addr & ~0xfff;
  //printf("tlb_manager: addr=%p satp=%p mmuid=%d\n", addr_no_mmuid, satp, mmuid);
  paddr pa = get_mapping(addr_no_mmuid, SATP2ASID(satp));

  w_tlbh(addr);
  //TODO all access rights for now
  uint16 prot =  PTE_R | PTE_W | PTE_X | PTE_V | ((SATP2ASID(satp) != 0) ? PTE_U : 0);
  w_tlbl(PA2PTE(pa) | prot);
  return;
}
