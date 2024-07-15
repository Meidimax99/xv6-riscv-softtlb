#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  __asm__("li x2, 42\n\t \
            li x1, 0x87fff000\n\t \
            sw x2, 0(x1)\n\t");
  exit(0);
}
