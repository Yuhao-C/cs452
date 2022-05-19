
#include "../user/include/boot.h"
#include "kern/common.h"
#include "kern/sys.h"
#include "kern/syscall.h"
#include "kern/task.h"
#include "lib/bwio.h"

int main() {
  // int a = 0;
  // bwprintf(COM2, "%x", &a);

  // store main's return address for exiting the kernel
  addr_t lr;
  asm volatile("mov %r0, lr" : "=r"(lr));
  exitAddr = lr;

  taskBootstrap();

  // add first user task
  Trapframe tf;
  tf.r0 = BOOT_PRIORITY;
  tf.r1 = (unsigned int)boot;
  taskCreate(&tf);

  while (true) {
    TaskDescriptor *nextTask = taskSchedule();
    bwprintf(COM2, "%d, %x, %d\n\r", nextTask->tid, nextTask->parent,
             nextTask->priority);
    int request = taskActivate(nextTask);
    (void)request;
    break;
    // if (!nextTask) break;
    // int request = taskActivate(nextTask);
    // (void)request;
    // handleRequest(request);
  }
  return 0;
}
