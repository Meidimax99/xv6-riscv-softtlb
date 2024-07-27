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

#define MAX_PROC_MEM(kernelend) ((PHYSTOP - PGROUNDUP(kernelend)) / NPROC)
#define PROC_START(procid, kernelend) (PGROUNDUP(kernelend) + (procid * MAX_PROC_MEM(kernelend)))
#define PROC_END(procid, kernelend) ((PGROUNDUP(kernelend) + ((procid + 1) * MAX_PROC_MEM(kernelend)))-1)
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
  for(int i = 0; i < NPROC; i++) {
    uint64 mem_start = PROC_START(i, (uint64)end);
    uint64 mem_end = PROC_END(i, (uint64)end);
    uint64 npages = (PROC_END(i, (uint64)end)- PROC_START(i, (uint64)end)) >> 12;
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
  freerange(end, (void*)(PHYSTOP-0x1000)); //TODO undo change -> just for testing
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

void *
kalloc(int procid)
{
  (void)procid;
  struct run *r;


  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  //modhere Dont fill with junk
  // if(r)
  //   memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
