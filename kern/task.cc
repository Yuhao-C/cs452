#include "task.h"

#include "common.h"
#include "syscall.h"

#define NUM_TASKS 64

#define SWI(n) "swi " TOSTRING(n)

TDEntry tasks[NUM_TASKS];
TDEntry *firstFreeEntry;

void taskBootstrap() {
  for (int i = 0; i < NUM_TASKS - 1; ++i) {
    tasks[i].nextFree = &tasks[i + 1];
  }
  tasks[NUM_TASKS - 1].nextFree = nullptr;
  firstFreeEntry = tasks;
}

void yield() { asm(SWI(SYS_yield)); }
