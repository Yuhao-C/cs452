#ifndef LIB_TIMER_H_
#define LIB_TIMER_H_

#include "kern/arch/ts7200.h"
namespace timer {

void load(unsigned int timerBase, unsigned int initialTimeMs);
void start(unsigned int timerBase);
void stop(unsigned int timerBase);
unsigned int getTick(unsigned int timerBase);

}  // namespace timer

#endif  // LIB_TIMER_H_
