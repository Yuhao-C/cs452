#include "kern/event.h"

#include "kern/task.h"
#include "lib/assert.h"

TaskDescriptor *eventQueues[NUM_EVENTS];

void eventBootstrap() {
  for (int i = 0; i < NUM_EVENTS; ++i) {
    eventQueues[i] = nullptr;
  }
}

void handleAwaitEvent() {
  int eventType = curTask->tf.r0;
  kAssert(0 <= eventType && eventType < NUM_EVENTS);
  curTask->state = TaskDescriptor::State::kEventBlocked;

  // enqueue task
  curTask->nextEventBlocked = nullptr;
  if (eventQueues[eventType]) {
    TaskDescriptor *tail = eventQueues[eventType];
    while (tail->nextEventBlocked) {
      tail = tail->nextEventBlocked;
    }
    tail->nextEventBlocked = curTask;
  } else {
    eventQueues[eventType] = curTask;
  }
}
