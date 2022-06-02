#include "kern/interrupt.h"

#include "kern/arch/ts7200.h"
#include "kern/event.h"
#include "kern/task.h"
#include "lib/assert.h"

void handleTC3UI() {
  // clear interrupt
  *(unsigned int *)(TIMER3_BASE + CLR_OFFSET) = 1;

  TaskDescriptor *awaitingTask = eventQueues[IRQ_TC3UI];
  if (!awaitingTask) {
    return;
  }
  kAssert(awaitingTask->state == TaskDescriptor::State::kEventBlocked);
  awaitingTask->tf.r0 = 0;
  awaitingTask->state = TaskDescriptor::State::kReady;
  readyQueues.enqueue(awaitingTask);
}