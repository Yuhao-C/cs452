#include "kern/task.h"
#include "lib/assert.h"

TaskDescriptor::TaskDescriptor(int parentTid, int priority, int tid)
    : tid{tid},
      parentTid{parentTid},
      priority{priority},
      nextReady{nullptr},
      sendQueue{},
      state{State::kReady},
      retVal{0} {}

TaskDescriptor::TaskDescriptor() : TaskDescriptor{-1, -1, -1} {}

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
