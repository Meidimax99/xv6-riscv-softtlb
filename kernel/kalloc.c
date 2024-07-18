// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

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

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)(PHYSTOP-0x1000)); //TODO undo change -> just for testing
  uint32 beef= 0xDEADBEEF;
  memsetStr((char*)0x84fff000, (char*) &beef, 4, PGSIZE); // fill with junk
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

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
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
