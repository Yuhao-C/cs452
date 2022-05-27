#include "lib/timer.h"

#include "kern/arch/ts7200.h"

namespace timer {

void load() { *(unsigned int *)(TIMER3_BASE + LDR_OFFSET) = 0xffffffff; }

void start() { *(unsigned int *)(TIMER3_BASE + CTRL_OFFSET) = 0b11001000; }

void stop() { *(unsigned int *)(TIMER3_BASE + CTRL_OFFSET) &= 0xffffff7f; }

unsigned int getTick() {
  unsigned int value = *(unsigned int *)(TIMER3_BASE + VAL_OFFSET);
  return value;
}

}  // namespace timer
