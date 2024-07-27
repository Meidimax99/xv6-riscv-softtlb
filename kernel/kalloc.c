// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

// Kernel Starts at KERNBASE 0x80000000

//Nproc + 1 because the #0 space is for kernel objects
#define MAX_PROC_MEM(kernelend) ((((PHYSTOP - (PGROUNDUP((uint64)kernelend))) / (NPROC + 1)) >> 12 )<< 12)
#define PROC_START(procid, kernelend) (PGROUNDUP((uint64)kernelend) + (procid * MAX_PROC_MEM((uint64)kernelend)))
#define PROC_N(index, procid, kernelend) (PROC_START(procid, kernelend) + (index * (0x1000)))
#define PROC_END(procid, kernelend) ((PGROUNDUP((uint64)kernelend) + ((procid + 1) * MAX_PROC_MEM((uint64)kernelend)))-0x1000)
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void*
memsetStr(void *dst, char *str, uint strlen, uint n)
{

  char *cdst = (char *) dst;
  uint i;
  for(i = 0; i < n; i++){
    cdst[i] = str[i % strlen];
  }
  return dst;
}

static char prefixes[5] = {'K', 'M', 'G', 'T', 'P'};

char maxPrefix(uint64 *num) {
  uint8 index = 0;
  while(*num > 1024) {
    index++;
    *num = *num >> 10;
  }
  return prefixes[index];
}
void printinfo() {
  printf("proc address ranges:\n\n");
  for(int i = 0; i < NPROC+1; i++) {
    uint64 mem_start = PROC_START(i, end);
    uint64 mem_end = PROC_END(i, end);
    uint64 npages = (mem_end - mem_start) >> 12;
    uint64 maxmem = npages * 4;
    char prefix = maxPrefix(&maxmem);
    printf("Proc %d:\n\tstart:%p\n\tend:%p\n\tmax pages: %d (%d%cB)\n\n",
        i, 
        mem_start, 
        mem_end, 
        npages, maxmem, prefix);
  }
}
void
kinit()
{ 
  printinfo();
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)(PROC_END(0, end)-0x1000)); //TODO undo change -> just for testing
  uint32 beef= 0xDEADBEEF;
  memsetStr((char*)0x85000000, (char*) &beef, 4, PGSIZE); // fill with junk
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start); //Dynamic kernel size possible!
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) //TODO Should the condition not be < instead of <= ?
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)

//TODO Change to fit kalloc()
void
kfree(void *pa)
{
  struct run *r;
  //Panic if:
  //pa is not page aligned
  //pa is in the kernel or below it
  //pa is bigger or equal to the last address in main mem
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

//Allocate a 4096 Byte page for the current proccess
//or for the kernel if procid < 0
//TODO per-process lock for memory
//TODO only for the processes for now
//TODO check cases were memory is not allocated from a process ( -> Disk ?)
//    proc_mapstacks!

#define NEXT_PAGE_INDEX(size) (size >> 12)

void *
kalloc(struct proc *p)
{
  if(p == 0) {
    //kernel
    //Maintain normal list for kernel because I do not know if the other calls from kalloc
    //come from a specific process -> possibly inconsistent process size
    struct run *r;
    acquire(&kmem.lock);
    r = kmem.freelist;
    if(r)
      kmem.freelist = r->next;
    release(&kmem.lock);
    return (void*)r;
  } else {
    uint64 size = p->sz;
    uint64 index = NEXT_PAGE_INDEX(size);
    uint64 *pa = (uint64*)PROC_N(index, p->pid, end);
    if((uint64)pa > PROC_END(p->pid, end)) {
      return 0;
    }
    printf("allocating for pid=%d page_index=%d pa=%p\n", p->pid, index, pa);
    return (void*)pa;
  }



  //modhere Dont fill with junk
  // if(r)
  //   memset((char*)r, 5, PGSIZE); // fill with junk
}
