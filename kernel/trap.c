#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

/* Exception causes */
//Stolen from Qemu  /target/riscv/cpu_bits.h
typedef enum RISCVException {
    NONE = -1, /* sentinel value */
    INST_ADDR_MIS = 0x0,
    INST_ACCESS_FAULT = 0x1,
    ILLEGAL_INST = 0x2,
    BREAKPOINT = 0x3,
    LOAD_ADDR_MIS = 0x4,
    LOAD_ACCESS_FAULT = 0x5,
    STORE_AMO_ADDR_MIS = 0x6,
    STORE_AMO_ACCESS_FAULT = 0x7,
    U_ECALL = 0x8,
    S_ECALL = 0x9,
    INST_PAGE_FAULT = 0xc, /* since: priv-1.10.0 */
    LOAD_PAGE_FAULT = 0xd, /* since: priv-1.10.0 */
    STORE_PAGE_FAULT = 0xf, /* since: priv-1.10.0 */
    TLB_MISS = 0x18
} Exception;

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void usertrap(void)
{
  uint64 scause = r_scause();
  int which_dev = 0;
  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  
  // save user program counter.
  p->trapframe->epc = r_sepc();
  //TODO interrupt handling
  switch (scause) {
    case U_ECALL: 
        // system call
        if(killed(p))
          exit(-1);

        // sepc points to the ecall instruction,
        // but we want to return to the next instruction.
        p->trapframe->epc += 4;

        // an interrupt will change sepc, scause, and sstatus,
        // so enable only now that we're done with those registers.
        intr_on();

        syscall();
        break;
    case TLB_MISS:
      //todo tlb miss is handled in machine mode, remove
        //tlb_handle_miss(r_stval());
        setkilled(p);
        break;
    case INST_ADDR_MIS:
    case INST_ACCESS_FAULT: 
    case ILLEGAL_INST: 
    case BREAKPOINT: 
    case LOAD_ADDR_MIS: 
    case LOAD_ACCESS_FAULT: 
    case STORE_AMO_ADDR_MIS: 
    case STORE_AMO_ACCESS_FAULT: 
    case S_ECALL: 
    case INST_PAGE_FAULT: 
    case LOAD_PAGE_FAULT: 
    case STORE_PAGE_FAULT: 
    default:
        if((which_dev = devintr()) != 0){
          // ok
        } else {
          printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
          printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
          setkilled(p);
        }
        break;
  }

  if(killed(p))
    exit(-1);

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2)
    yield();

  usertrapret();
}

//
// return to user space
//
void
usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to uservec in trampoline.S
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  // set up trapframe values that uservec will need when
  // the process next traps into the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to userret in trampoline.S at the top of memory, which 
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64))trampoline_userret)(satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{

  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  if((sstatus & SSTATUS_SPP) == 0) {
    printf("Kerneltrap with scause=0x%x\n",scause);
    panic("kerneltrap: not from supervisor mode");
  }
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    printf("scause %p\n", scause);
    printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  acquire(&tickslock);
  ticks++;
  wakeup(&ticks);
  release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if((scause & 0x8000000000000000L) &&
     (scause & 0xff) == 9){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000001L){
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    if(cpuid() == 0){
      clockintr();
    }
    
    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  } else {
    return 0;
  }
}

void unhandled_exc(uint64 mcause, uint64 mtval) {
  printf("Unhandled machine-mode exception!\n"
         "mcause=%p\tmtval?=%p\n\n", mcause, mtval);
}


void machine_default_exception_handler() {
  switch (r_mcause()) {
    case NONE:
    case INST_ADDR_MIS:
    case INST_ACCESS_FAULT:
    case ILLEGAL_INST:
    case BREAKPOINT:
    case LOAD_ADDR_MIS:
    case LOAD_ACCESS_FAULT:
    case STORE_AMO_ADDR_MIS:
    case STORE_AMO_ACCESS_FAULT:
    case U_ECALL:
    case S_ECALL:
    case INST_PAGE_FAULT:
    case LOAD_PAGE_FAULT:
    case STORE_PAGE_FAULT:
    default:
        unhandled_exc(r_mcause(), r_mtval());
        break;
    case TLB_MISS:
        tlb_handle_miss(r_mtval(), r_satp());
        break;
}

}
