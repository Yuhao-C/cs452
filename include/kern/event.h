#ifndef KERN_EVENT_H_
#define KERN_EVENT_H_

#include "kern/syscall.h"

#define NUM_EVENTS 64

struct TaskDescriptor;

extern TaskDescriptor *eventQueues[NUM_EVENTS];

void eventBootstrap();

void handleAwaitEvent();

#endif  // KERN_EVENT_H_