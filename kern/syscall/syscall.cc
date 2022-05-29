#include "kern/syscall.h"

#include "kern/message.h"
#include "kern/task.h"
#include "lib/assert.h"
#include "lib/bwio.h"

extern "C" void enterKernel(Trapframe *tf, unsigned int code) {
  curTask->tf = *tf;

  code &= 0xffffff;
  switch (code) {
    case SYS_CREATE:
      taskCreate(&curTask->tf);
      taskYield();
      break;
    case SYS_TID:
      curTask->tf.r0 = curTask->tid;
      taskYield();
      break;
    case SYS_PARENT_TID:
      curTask->tf.r0 = curTask->parent ? curTask->parent->tid : -1;
      taskYield();
      break;
    case SYS_YIELD:
      taskYield();
      break;
    case SYS_EXIT:
      taskExit();
      break;
    case SYS_SEND:
      msgSend(&curTask->tf);
      break;
    case SYS_RECEIVE:
      msgReceive(&curTask->tf);
      break;
    case SYS_REPLY:
      msgReply(&curTask->tf);
      break;
    default:
      bwprintf(COM2,
               "\033[31m"
               "FATAL: unknown syscall code: %u\n\r"
               "\033[0m",
               code);
      kAssert(false);
  }
}

void leaveKernel() { userMode(&curTask->tf); }