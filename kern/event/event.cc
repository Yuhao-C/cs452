#include "kern/event.h"

#include "kern/task.h"
#include "lib/assert.h"

EventBuffer::EventBuffer() : head{nullptr} {}

void EventBuffer::push(TaskDescriptor *task) {
  task->nextEventBlocked = head;
  head = task;
}

TaskDescriptor *EventBuffer::pop() {
  TaskDescriptor *temp = head;
  if (temp) {
    head = temp->nextEventBlocked;
  }
  return temp;
}

EventBuffer eventBuffers[NUM_EVENTS];

void eventBootstrap() {
  for (int i = 0; i < NUM_EVENTS; ++i) {
    eventBuffers[i] = EventBuffer();
  }
}

void handleAwaitEvent() {
  int eventType = curTask->tf.r0;
  kAssert(0 <= eventType && eventType < NUM_EVENTS);
  curTask->state = TaskDescriptor::State::kEventBlocked;

  // push task
  eventBuffers[eventType].push(curTask);
}
