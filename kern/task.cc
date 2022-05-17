#include "task.h"

#include "common.h"
#include "lib/sys.h"
#include "task_private.h"

#define SWI(n) "swi " TOSTRING(n)

TaskDescriptor tasks[NUM_TASKS];

PriorityQueues priorityQueues;

TaskDescriptor *curTask;

int tidCounter;

void taskBootstrap() {
  tidCounter = 0;
  priorityQueues = PriorityQueues();
}

int myTid() { return curTask->tid; }

int myParentTid() { return curTask->parent->tid; }

void yield() { taskSwitch(TaskDescriptor::State::kReady); }

void exit() { taskSwitch(TaskDescriptor::State::kZombie); }

void taskSwitch(TaskDescriptor::State newState) {
  TaskDescriptor *cur = curTask;
  TaskDescriptor *next = priorityQueues.dequeue();

  cur->state = newState;

  if (!next) {
    if (newState == TaskDescriptor::State::kZombie) {
      kExit();
    }
    return;
  }

  priorityQueues.enqueue(cur);

  switchFrame(next->stackPtr, &cur->stackPtr);

  curTask = cur;
  cur->state = TaskDescriptor::State::kActive;
}

void switchFrame(addr_t spNew, addr_t *spOld) {
  asm(R"(
    nop
    sub sp, sp, #40
    str r4, [sp, #0]
    mrs r4, CPSR
    str r5, [sp, #4]
    str r6, [sp, #8]
    str r7, [sp, #12]
    str r8, [sp, #16]
    str r9, [sp, #20]
    str r10, [sp, #24]
    str r11, [sp, #28]
    str lr, [sp, #32]
    str r4, [sp, #36]

    str sp, [r1, #0]

    mov sp, r0
    nop

    ldr r11, [sp, #36]
    ldr r4, [sp, #0]
    ldr r5, [sp, #4]
    ldr r6, [sp, #8]
    ldr r7, [sp, #12]
    ldr r8, [sp, #16]
    ldr r9, [sp, #20]
    ldr r10, [sp, #24]
    msr CPSR_cxsf, r11
    ldr r11, [sp, #28]
    ldr lr, [sp, #32]
    nop

    add sp, sp, #40
    nop
  )");
}