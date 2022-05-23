
#include "../user/include/boot.h"
#include "kern/common.h"
#include "kern/sys.h"
#include "kern/syscall.h"
#include "kern/task.h"
#include "lib/bwio.h"

int main() {
  // store main's return address for exiting the kernel
  addr_t lr;
  asm volatile("mov %r0, lr" : "=r"(lr));

  sysBootstrap(lr);
  taskBootstrap();

  // add first user task
  Trapframe tf;
  tf.r0 = BOOT_PRIORITY;
  tf.r1 = (unsigned int)boot;
  taskCreate(&tf);

  while (true) {
    TaskDescriptor *nextTask = taskSchedule();
    if (!nextTask) break;
    int request = taskActivate(nextTask);
    (void)request;
    // handleRequest(request);
  }
  return 0;
}
