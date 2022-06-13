#include "kern/task.h"
#include "lib/assert.h"

TaskDescriptor::TaskDescriptor(TaskDescriptor *parent, int priority, int tid)
    : tid{tid},
      parent{parent},
      priority{priority},
      nextReady{nullptr},
      sendQueue{},
      state{State::kReady},
      retVal{0} {}

TaskDescriptor::TaskDescriptor() : TaskDescriptor{nullptr, -1, -1} {}

void TaskDescriptor::enqueueSender(TaskDescriptor *sender) {
  kAssert(sender != nullptr);
  sendQueue.enqueue(sender);
}

TaskDescriptor *TaskDescriptor::dequeueSender() {
  if (sendQueue.size() > 0) {
    return sendQueue.dequeue();
  }
  return nullptr;
}
