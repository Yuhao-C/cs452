#include "lib/timer.h"

#include "kern/arch/ts7200.h"

namespace timer {

void load(unsigned int timerBase, unsigned int initialTimeMs) {
  volatile unsigned int *addr = (unsigned int *)(timerBase + LDR_OFFSET);
  *addr = initialTimeMs * (TIMER3_FRQ / 1000);
}

void start(unsigned int timerBase) {
  volatile unsigned int *addr = (unsigned int *)(timerBase + CTRL_OFFSET);
  *addr = 0b11001000;
}

void stop(unsigned int timerBase) {
  volatile unsigned int *addr = (unsigned int *)(timerBase + CTRL_OFFSET);
  *addr &= 0x7f;
}

unsigned int getTick(unsigned int timerBase) {
  volatile unsigned int *addr = (unsigned int *)(timerBase + VAL_OFFSET);
  return *addr;
}

}  // namespace timer
