#include "kern/task.h"
#include "lib/assert.h"

PriorityQueues::PriorityQueues() {
  for (int i = 0; i < NUM_PRIORITY_LEVELS; ++i) {
    heads[i] = nullptr;
    tails[i] = nullptr;
  }
}

void PriorityQueues::enqueue(TaskDescriptor *task) {
  int priority = task->priority;
  assert(0 <= priority && priority < NUM_PRIORITY_LEVELS);
  if (!heads[priority]) {
    assert(!tails[priority]);
    heads[priority] = task;
  } else {
    assert(tails[priority]);
    tails[priority]->nextReady = task;
  }
  tails[priority] = task;
  task->nextReady = nullptr;
}

TaskDescriptor *PriorityQueues::dequeue(int priority) {
  assert(0 <= priority && priority < NUM_PRIORITY_LEVELS);
  TaskDescriptor *h = heads[priority];
  if (h) {
    heads[priority] = h->nextReady;
    if (!heads[priority]) {
      tails[priority] = nullptr;
    }
  }
  return h;
}

TaskDescriptor *PriorityQueues::dequeue() {
  for (int i = 0; i < NUM_PRIORITY_LEVELS; ++i) {
    TaskDescriptor *h = dequeue(i);
    if (h) {
      return h;
    }
  }
  return nullptr;
}

bool PriorityQueues::isEmpty(int priority) {
  assert(0 <= priority && priority < NUM_PRIORITY_LEVELS);
  return heads[priority];
}
