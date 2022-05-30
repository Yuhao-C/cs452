
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

#if ENABLE_CACHE
  // clean and invalidate cache
  asm volatile("mcr p15, 0, r0, c7, c14");  // clean and invalidate d-cache
  asm volatile("mcr p15, 0, r0, c7, c5");   // invalidate i-cache

  // enable cache
  unsigned int c1;
  asm volatile("mrc p15, 0, %r0, c1, c0" : "=r"(c1));
  c1 |= 0b0001'0000'0000'0100;
  asm volatile("mcr p15, 0, %r0, c1, c0" : : "r"(c1));
#endif

  // add first user task
  Trapframe tf;
  tf.r0 = BOOT_PRIORITY;
  tf.r1 = (unsigned int)boot;
  taskCreate(&tf);

  while (true) {
    TaskDescriptor *nextTask = taskSchedule();
    if (!nextTask) {
      break;
    }
    int request = taskActivate(nextTask);
    (void)request;
    // handleRequest(request);
  }
  return 0;
}
