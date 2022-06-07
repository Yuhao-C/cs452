#include "kern/syscall.h"

#include "kern/arch/ts7200.h"
#include "kern/event.h"
#include "kern/interrupt.h"
#include "kern/message.h"
#include "kern/sys.h"
#include "kern/task.h"
#include "lib/assert.h"
#include "lib/bwio.h"
#include "lib/math.h"
#include "lib/timer.h"

#define TIMER2_INIT_MS 100

extern "C" {
void trap(Trapframe *tf) {
  if (curTask->priority == 7 && curTask->nextReady == nullptr) {
    int temp = 508 * TIMER2_INIT_MS - timer::getTick(TIMER2_BASE);
    idleTime += temp;
  }
  curTask->tf = *tf;
}

unsigned int getIrqStatus() {
  unsigned int vic1Status =
      *(volatile unsigned int *)(VIC1_BASE + IRQ_STATUS_OFFSET);
  unsigned int vic2Status =
      *(volatile unsigned int *)(VIC2_BASE + IRQ_STATUS_OFFSET);
  kAssert(vic1Status != 0 || vic2Status != 0);

  unsigned int result = -1;
  if (vic1Status != 0) {
    result = log2(vic1Status);
  } else {
    result = log2(vic2Status) + 32;
  }
  kAssert(0 <= result && result < 64);
  // bwprintf(COM2, "irq: %d\n\r", result);
  return result;
}

void enterKernel(unsigned int code) {
  code &= 0xffffff;

  switch (code) {
    case IRQ_TC3UI:
      handleTC3UI();
      taskYield();
      break;
    case IRQ_UART1:
    case IRQ_UART2:
      handleUART(code);
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
      msgSend();
      break;
    case SYS_RECEIVE:
      msgReceive();
      break;
    case SYS_REPLY:
      msgReply();
      break;
    case SYS_AWAIT_EVENT:
      handleAwaitEvent();
      break;
    case SYS_IDLE_TIME:
      curTask->tf.r0 = idleTime / 5080;
      taskYield();
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
}

void leaveKernel() {
  if (curTask->priority == 7 && curTask->nextReady == nullptr) {
    timer::stop(TIMER2_BASE);
    timer::load(TIMER2_BASE, TIMER2_INIT_MS);
    timer::start(TIMER2_BASE);
  }
  userMode(&curTask->tf);
}