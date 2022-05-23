#include "kern/sys.h"

addr_t exitAddr;

void sysBootstrap(addr_t lr) {
  exitAddr = lr;
  // make suer 0x08 holds the correct instruction for SWI
  *((unsigned int *)0x08) = 0xe59ff018;
}

void kExit() {
  asm volatile(
      "mov lr, %[value]\n\t"
      "bx lr"
      :
      : [value] "r"(exitAddr)
      : "r14");
}