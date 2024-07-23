#include "kernel/types.h"
#include "user/user.h"

void do_tlb_exc(void) {
  __asm__("li s2, 0x84fff000\n\t \
          lw s4, 0(s2)\n\t"); //Weird output here when context jumps back with other reg contents -> not state restore!
  register int *foo asm ("s4");
  printf("%x\n", foo);
  return;
}
int main(int argc, char *argv[]) {
  do_tlb_exc();
  //exit(0);
}
