#include "kernel/types.h"
#include "user/user.h"
int
main(int argc, char *argv[])
{
  __asm__("unimp\n\t");
  exit(0);
}
