#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  __asm__("li x1, 0x87fff000\n\t \
            lw x2, 0(x1)\n\t"); //Weird output here when context jumps back with other reg contents -> not state restore!
  exit(0);
}
