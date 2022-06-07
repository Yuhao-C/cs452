#ifndef KERN_EVENT_H_
#define KERN_EVENT_H_

#include "kern/syscall.h"

#define NUM_EVENTS 64

struct TaskDescriptor;

class EventBuffer {
  TaskDescriptor *head;

 public:
  EventBuffer();
  void push(TaskDescriptor *task);
  TaskDescriptor *pop();
};

extern EventBuffer eventBuffers[NUM_EVENTS];

void eventBootstrap();

void handleAwaitEvent();

#endif  // KERN_EVENT_H_