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



//Nproc + 1 because the #0 space is for kernel objects
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

//#0 is conceptually for the kernel but not used
int address_spaces[NPROC+1];


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
  printf("address ranges:\n\n");
  for(int i = 0; i < NPROC+1; i++) {
    uint64 mem_start = AS_START(i);
    uint64 mem_end = AS_END(i);
    uint64 npages = (mem_end - mem_start) >> 12;
    uint64 maxmem = npages * 4;
    char prefix = maxPrefix(&maxmem);
    if(i == 0) {
      printf("Kernel:\n\tstart:%p\n\tend:%p\n\tmax pages: %d (%d%cB)\n\n",
        mem_start, 
        mem_end, 
        npages, maxmem, prefix);
    } else {
      printf("Address Space %d:\n\tstart:%p\n\tend:%p\n\tmax pages: %d (%d%cB)\n\n",
          i, 
          mem_start, 
          mem_end, 
          npages, maxmem, prefix);
    }
  }
}

void initAddressSpaces(void) {
  for(int i = 0; i < NPROC+1; i++) {
    address_spaces[i] = 0;
  }
}
//TODO LOCKING
int getAddressSpace(int pid) {
  for(int i = 1; i < NPROC+1; i++) {
    if(address_spaces[i] == 0) {
      address_spaces[i] = pid;
      return i;
    }
  }
  return -1;
}
//TODO LOCKING

int freeAddressSpace(int pid, int asid) {
  if(asid != 0 && address_spaces[asid] == pid) {
    address_spaces[asid] = 0;
    return 0;
  }
  return -1;
}

void
kinit()
{ 
  initAddressSpaces();
  printinfo();
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)(KERNEL_MEM_END)); //TODO undo change -> just for testing
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
  //memset(pa, 1, PGSIZE);

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
kalloc()
{

  struct run *r;
  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);
  //printf("kernel palloc: %p\n", (void*)r);
  return (void*)r;
}
