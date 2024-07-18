#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  __asm__("li s2, 0x84fff000\n\t \
          li s3, 42\n\t \
          sw s3, 0(s2)\n\t \
          lw s4, 0(s2)\n\t"); //Weird output here when context jumps back with other reg contents -> not state restore!
  register int *foo asm ("s4");
  printf("%d\n", foo);
  exit(0);
}
