
#ifndef KERN_SYSCALL_H_
#define KERN_SYSCALL_H_

#include "syscall_code.h"

struct Trapframe {
  unsigned int lrSVC;
  unsigned int spsr;
  unsigned int r0;
  unsigned int r1;
  unsigned int r2;
  unsigned int r3;
  unsigned int r4;
  unsigned int r5;
  unsigned int r6;
  unsigned int r7;
  unsigned int r8;
  unsigned int r9;
  unsigned int r10;
  unsigned int r11;
  unsigned int r12;
  unsigned int r13;
  unsigned int r14;

  void print() const;
};

extern "C" {
void handleSWI();

void handleIRQ();

void enterKernel(unsigned int code);

void userMode(Trapframe *tf);
}

void leaveKernel();

#endif  // KERN_SYSCALL_H_