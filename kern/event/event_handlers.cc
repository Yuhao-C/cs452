#include "kern/arch/ts7200.h"
#include "kern/event.h"
#include "kern/interrupt.h"
#include "kern/task.h"
#include "lib/assert.h"

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