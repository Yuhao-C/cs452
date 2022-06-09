#include "kern/arch/ts7200.h"
#include "kern/event.h"
#include "kern/interrupt.h"
#include "kern/task.h"
#include "lib/assert.h"
#include "lib/bwio.h"

void clearEventBuffer(int eventType, int retVal) {
  TaskDescriptor *awaitingTask = eventBuffers[eventType].pop();
  while (awaitingTask) {
    kAssert(awaitingTask->state == TaskDescriptor::State::kEventBlocked);
    awaitingTask->tf.r0 = retVal;
    awaitingTask->state = TaskDescriptor::State::kReady;
    readyQueues.enqueue(awaitingTask);
    awaitingTask = eventBuffers[eventType].pop();
  }
}

void handleTC3UI() {
  // clear tc3 interrupt
  *(volatile unsigned int *)(TIMER3_BASE + CLR_OFFSET) = 1;
  clearEventBuffer(IRQ_TC3UI, 0);
}

void handleUART(int eventType) {
  kAssert(eventType == IRQ_UART1 || eventType == IRQ_UART2);
  unsigned int base = eventType == IRQ_UART1 ? UART1_BASE : UART2_BASE;
  volatile unsigned int *intr = (unsigned int *)(base + UART_INTR_OFFSET);
  volatile unsigned int *ctrl = (unsigned int *)(base + UART_CTRL_OFFSET);

  unsigned int val = *intr;
  if (val & RTIS_MASK || val & RIS_MASK) {
    *ctrl &= ~RIEN_MASK;
    *ctrl &= ~RTIEN_MASK;
  }
  if (val & TIS_MASK) {
    *ctrl &= ~TIEN_MASK;
  }
  if (val & MIS_MASK) {
    *intr = 0;
  }
  clearEventBuffer(eventType, val);
}
