#include "lib/sys.h"

addr_t exitAddr;

void exit() {
  asm("mov lr, %[value]\n\t"
      "bx lr"
      :
      : [value] "r"(exitAddr)
      : "r14");
}