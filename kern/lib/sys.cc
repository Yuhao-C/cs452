#include "kern/sys.h"

#include "kern/arch/ts7200.h"
#include "kern/syscall.h"
#include "lib/bwio.h"
#include "lib/timer.h"

#define SWI_ENTRY (volatile unsigned int *)0x08
#define IRQ_ENTRY (volatile unsigned int *)0x18
#define SWI_HANDLER (volatile addr_t *)0x28
#define IRQ_HANDLER (volatile addr_t *)0x38

addr_t exitAddr;

unsigned int idleTime;

void sysBootstrap(addr_t lr) {
  idleTime = 0;
  exitAddr = lr;

  // make suer 0x08 holds the correct instruction for SWI
  *SWI_ENTRY = 0xe59ff018;
  // make suer 0x18 holds the correct instruction for IRQ
  *IRQ_ENTRY = 0xe59ff018;

  // set up SWI and IRQ handler entry
  *SWI_HANDLER = (addr_t)handleSWI;
  *IRQ_HANDLER = (addr_t)handleIRQ;

  // enable interrupts
  *(volatile unsigned int *)(VIC1_BASE + INT_ENABLE_OFFSET) = 0;
  *(volatile unsigned int *)(VIC2_BASE + INT_ENABLE_OFFSET) = 0x80000;

  // enable Halt
  *(volatile unsigned int *)(SYS_SW_LOCK) = 0xaa;
  *(volatile unsigned int *)(DEVICE_CFG) |= 1;
}

void kExit() {
  timer::stop(TIMER3_BASE);

  asm volatile(
      "mov lr, %[value]\n\t"
      "bx lr"
      :
      : [value] "r"(exitAddr)
      : "r14");
}