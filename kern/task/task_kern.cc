#include "kern/common.h"
#include "kern/sys.h"
#include "kern/task.h"
#include "lib/bwio.h"
#include "user/task.h"
#include "lib/queue.h"

#define USER_STACK_START 0x1000000

TaskDescriptor tasks[NUM_TASKS];
TaskDescriptor *curTask;

PriorityQueues readyQueues;
Queue<int, 64> tidPool;

void taskBootstrap() {
  for (int i = 0; i < NUM_TASKS; ++i) {
    tasks[i] = TaskDescriptor{};
  }
  curTask = nullptr;
  readyQueues = PriorityQueues();
  tidPool = Queue<int, 64>{};
  for (int i = 0; i < NUM_TASKS; ++i) {
    bool enqueueDone = tidPool.enqueue(i);
    kAssert(enqueueDone);
  }
}

void taskStart(void (*fn)()) {
  fn();
  destroy();
}

void taskCreate(Trapframe *tf) {
  int priority = tf->r0;
  auto fn = tf->r1;
  if (priority < 0 || priority >= NUM_PRIORITY_LEVELS) {
    tf->r0 = -1;
    return;
  }
  if (tidPool.size() == 0) {
    tf->r0 = -2;
    return;
  }

  int tid = tidPool.dequeue();
  int index = tid % NUM_TASKS;
  addr_t stack = USER_STACK_START - index * USER_STACK_SIZE;
  TaskDescriptor &task = tasks[index];
  kAssert(!isTidValid(tid)); // tid must be invalid at this point
  task = TaskDescriptor{curTask->tid, priority, tid};
  task.tf.r0 = fn;
  task.tf.r11 = stack;  // frame pointer
  task.tf.r13 = stack;  // stack pointer
  task.tf.lrSVC = (unsigned int)taskStart;
  task.tf.spsr = 0b10000;
  readyQueues.enqueue(&task);
  tf->r0 = task.tid;  // return tid in r0
}

void taskYield() {
  curTask->state = TaskDescriptor::State::kReady;
  readyQueues.enqueue(curTask);
}

void taskExit() { curTask->state = TaskDescriptor::State::kZombie; }

void taskDestroy() {
  tidPool.enqueue(curTask->tid + NUM_TASKS);
  curTask->tid = -1;
  curTask->state = TaskDescriptor::State::kZombie;
}

int taskActivate(TaskDescriptor *task) {
  curTask = task;
  task->state = TaskDescriptor::State::kActive;
  leaveKernel();

  // after enterKernel
  return 0;
}

TaskDescriptor *taskSchedule() { return readyQueues.dequeue(); }

TaskDescriptor *getTd(int tid) {
  if (tid == -1) {
    return nullptr;
  }
  TaskDescriptor *td = &tasks[tid % NUM_TASKS];
  if (td->tid == tid) {
    return td;
  }
  return nullptr;
}

bool isTidValid(int tid) {
  return getTd(tid) != nullptr;
}
