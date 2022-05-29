#include "kern/syscall.h"
#include "lib/bwio.h"

void Trapframe::print() const {
  bwprintf(COM2, "r0: 0x%x\n\r", r0);
  bwprintf(COM2, "r1: 0x%x\n\r", r1);
  bwprintf(COM2, "r2: 0x%x\n\r", r2);
  bwprintf(COM2, "r3: 0x%x\n\r", r3);
  bwprintf(COM2, "r4: 0x%x\n\r", r4);
  bwprintf(COM2, "r5: 0x%x\n\r", r5);
  bwprintf(COM2, "r6: 0x%x\n\r", r6);
  bwprintf(COM2, "r7: 0x%x\n\r", r7);
  bwprintf(COM2, "r8: 0x%x\n\r", r8);
  bwprintf(COM2, "r9: 0x%x\n\r", r9);
  bwprintf(COM2, "r10: 0x%x\n\r", r10);
  bwprintf(COM2, "r11: 0x%x\n\r", r11);
  bwprintf(COM2, "r12: 0x%x\n\r", r12);
  bwprintf(COM2, "r13: 0x%x\n\r", r13);
  bwprintf(COM2, "r14: 0x%x\n\r", r14);
  bwprintf(COM2, "spsr: 0x%x\n\r", spsr);
  bwprintf(COM2, "lrSVC: 0x%x\n\r", lrSVC);
}