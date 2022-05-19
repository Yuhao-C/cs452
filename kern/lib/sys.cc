#include "kern/sys.h"

addr_t exitAddr;

void kExit() {
  asm volatile(
      "mov lr, %[value]\n\t"
      "bx lr"
      :
      : [value] "r"(exitAddr)
      : "r14");
}