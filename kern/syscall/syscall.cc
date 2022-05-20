#include "kern/syscall.h"

#include "kern/task.h"
#include "lib/assert.h"
#include "lib/bwio.h"

extern "C" void enterKernel(Trapframe *tf, unsigned int code) {
  curTask->tf = *tf;

  code &= 0xffffff;
  switch (code) {
    case SYS_CREATE:
      taskCreate(&curTask->tf);
      break;
    case SYS_TID:
      tf->r0 = curTask->tid;
      break;
    case SYS_PARENT_TID:
      tf->r0 = curTask->parent ? curTask->parent->tid : -1;
      break;
    case SYS_YIELD:
      taskYield();
      break;
    case SYS_EXIT:
      taskExit();
      break;
    default:
      bwprintf(COM2,
               "\033[31m"
               "FATAL: unknown syscall code: %u\n\r"
               "\033[0m",
               code);
      assert(false);
  }
}

void leaveKernel() { userMode(&curTask->tf); }