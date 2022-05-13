#include "lib/sys.h"

addr_t exitAddr;

void sysExit() {
  asm("mov lr, %[value]\n\t"
      "bx lr"
      :
      : [value] "r"(exitAddr)
      : "r14");
}