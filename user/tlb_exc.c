#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  __asm__("li x1, 0x88000000\n\t \
            lw x2, 0(x1)\n\t");
  exit(0);
}
