#include "kernel/types.h"
#include "user/user.h"

void fail_print(uint64 i) {
  fprintf(2, "Error: Call to sbrk failed for arg: %p!\n",(uint64*)i);
}

int main(int argc, char *argv[]) {
  uint64 current = (uint64)sbrk(0);
  if(current == -1) {
    fail_print(0);
  }
  fprintf(2, "Current size: %p pages\n", current >> 12);
  if(argc == 2){
    uint64 page_count = atoi(argv[1]);
    uint64 old_brk = current;
    for(uint64 i = 0; i < page_count; i++) {
      old_brk = (uint64)sbrk(4096);
      if(old_brk == -1) {
        fail_print(4096);
        break;
      }
      current = old_brk + 4096;
    }
  } else {
    exit(0);
  }
  fprintf(2, "Current size: %p pages\n", current >> 12);
}
