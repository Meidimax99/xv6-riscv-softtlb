#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  __asm__("li x1, 0x84fff000\n\t \
          li x2, 42\n\t \
          sw x2, 0(x1)\n\t \
          lw x3, 0(x1)\n\t"); //Weird output here when context jumps back with other reg contents -> not state restore!
  register int foo asm ("x3");
  printf("%d\n", foo);
  exit(0);
}
