#ifndef KERN_TASK_PRIVATE_H_
#define KERN_TASK_PRIVATE_H_

#include "common.h"

#define NUM_TASKS 64
#define NUM_PRIORITY_LEVELS 8

extern int tidCounter;

struct TaskDescriptor {
  enum class State {
    kActive = 0,
    kReady,
    kZombie,
    kSendBlocked,
    kReceiveBlocked,
    kReplyBlocked,
    kEventBlocked
  };

  int tid;
  TaskDescriptor *parent;
  int priority;
  TaskDescriptor *nextReady;
  TaskDescriptor *nextSend;
  State state;
  addr_t stackPtr;
  int retVal;
  unsigned int spsr;  // TODO: somehow determine save this or save cpsr

  TaskDescriptor(TaskDescriptor *parent, int priority);
  TaskDescriptor();
};

class PriorityQueues {
  TaskDescriptor *heads[NUM_PRIORITY_LEVELS];
  TaskDescriptor *tails[NUM_PRIORITY_LEVELS];

 public:
  PriorityQueues();
  void enqueue(TaskDescriptor *task);
  TaskDescriptor *dequeue(int priority);
  TaskDescriptor *dequeue();
  bool isEmpty(int priority);
};

void taskBootstrap();

void taskSwitch(TaskDescriptor::State newState);

void switchFrame(addr_t spNew, addr_t *spOld);

#endif  // KERN_TASK_PRIVATE_H_
