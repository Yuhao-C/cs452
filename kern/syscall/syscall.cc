#include "kern/syscall.h"

#include "kern/arch/ts7200.h"
#include "kern/message.h"
#include "kern/task.h"
#include "lib/assert.h"
#include "lib/bwio.h"
#include "lib/math.h"

extern "C" {
void trap(Trapframe *tf) { curTask->tf = *tf; }

unsigned int getIrqStatus() {
  unsigned int vic1Status = *(unsigned int *)(VIC1_BASE + IRQ_STATUS_OFFSET);
  unsigned int vic2Status = *(unsigned int *)(VIC2_BASE + IRQ_STATUS_OFFSET);
  kAssert(vic1Status != 0 || vic2Status != 0);

  unsigned int result = -1;
  if (vic1Status != 0) {
    result = log2(vic1Status);
  } else {
    result = log2(vic2Status) + 32;
  }
  kAssert(0 <= result && result < 64);

  bwprintf(COM2, "IRQ status: %d\n\r", result);

  return result;
}

void enterKernel(unsigned int code) {
  code &= 0xffffff;

  switch (code) {
    case IRQ_TC3UI:
      *(unsigned int *)(TIMER3_BASE + CLR_OFFSET) = 1;
      taskYield();
      break;
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
      taskYield();
  }
}
}

void leaveKernel() { userMode(&curTask->tf); }