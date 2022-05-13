#ifndef KERN_TASK_H_
#define KERN_TASK_H_

typedef unsigned int addr_t;

class TaskDescriptor {
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
};

int create(int priority, void (*function)());

int myTid();

int myParentTid();

void yield();

void exit();

void destroy();

#endif  // KERN_TASK_H_
