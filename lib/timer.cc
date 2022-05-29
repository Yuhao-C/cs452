#include "lib/timer.h"

#include "kern/arch/ts7200.h"

namespace timer {

void load() {
  volatile unsigned int *addr = (unsigned int *)(TIMER3_BASE + LDR_OFFSET);
  *addr = 0xffffffff;
}

void start() {
  volatile unsigned int *addr = (unsigned int *)(TIMER3_BASE + CTRL_OFFSET);
  *addr = 0b11001000;
}

void stop() {
  volatile unsigned int *addr = (unsigned int *)(TIMER3_BASE + CTRL_OFFSET);
  *addr &= 0xffffff7f;
}

unsigned int getTick() {
  volatile unsigned int *addr = (unsigned int *)(TIMER3_BASE + VAL_OFFSET);
  return *addr;
}

}  // namespace timer
