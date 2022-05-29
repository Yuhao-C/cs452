#include "kern/task.h"
#include "lib/assert.h"

TaskDescriptor::TaskDescriptor(TaskDescriptor *parent, int priority, int tid)
    : tid{tid},
      parent{parent},
      priority{priority},
      nextReady{nullptr},
      nextSend{nullptr},
      lastSend{nullptr},
      state{State::kReady},
      retVal{0} {}

TaskDescriptor::TaskDescriptor() : TaskDescriptor{nullptr, -1, -1} {}

void TaskDescriptor::enqueueSender(TaskDescriptor *sender) {
  if (!nextSend) {
    // empty send queue
    kAssert(!lastSend);
    nextSend = sender;
  } else {
    kAssert(lastSend);
    lastSend->nextSend = sender;
  }
  lastSend = sender;
  sender->nextSend = nullptr;
}

TaskDescriptor *TaskDescriptor::dequeueSender() {
  TaskDescriptor *h = nextSend;
  if (h) {
    nextSend = h->nextSend;
    if (!nextSend) {
      // send queue is empty
      lastSend = nullptr;
    }
  }
  return h;
}
