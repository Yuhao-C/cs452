#include "kern/common.h"
#include "kern/syscall.h"
#include "kern/task.h"
#include "user/task.h"

#define SWI(n) "swi " TOSTRING(n)

TaskDescriptor tasks[NUM_TASKS];
TaskDescriptor *curTask;

PriorityQueues priorityQueues;

int tidCounter;
addr_t userStack;

void taskBootstrap() {
  for (int i = 0; i < NUM_TASKS; ++i) {
    tasks[i] = TaskDescriptor{};
  }
  curTask = nullptr;
  tidCounter = 0;
  userStack = 0x1000000;
  priorityQueues = PriorityQueues();
  addr_t *swiHandler = (addr_t *)0x28;
  *swiHandler = (addr_t)handleSWI;
}

void taskStart(void (*fn)()) {
  fn();
  exit();
}

void taskCreate(Trapframe *tf) {
  int priority = tf->r0;
  auto fn = tf->r1;
  if (priority < 0 || priority >= NUM_PRIORITY_LEVELS) {
    tf->r0 = -1;
    return;
  }
  if (tidCounter >= NUM_TASKS) {
    tf->r0 = -2;
    return;
  }

  TaskDescriptor &task = tasks[tidCounter];
  task = TaskDescriptor{curTask, priority, tidCounter};
  task.tf.r0 = fn;
  task.tf.r11 = userStack;  // frame pointer
  task.tf.r13 = userStack;  // stack pointer
  task.tf.lrSVC = (unsigned int)taskStart;
  task.tf.spsr = 0b10000;
  priorityQueues.enqueue(&task);

  tf->r0 = task.tid;  // return tid in r0

  ++tidCounter;
  userStack -= USER_STACK_SIZE;
}

void taskYield() {
  curTask->state = TaskDescriptor::State::kReady;
  priorityQueues.enqueue(curTask);
}

void taskExit() { curTask->state = TaskDescriptor::State::kZombie; }

int taskActivate(TaskDescriptor *task) {
  curTask = task;
  task->state = TaskDescriptor::State::kActive;
  leaveKernel();

  // after enterKernel
  return 0;
}

TaskDescriptor *taskSchedule() { return priorityQueues.dequeue(); }
