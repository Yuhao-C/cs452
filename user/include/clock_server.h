#ifndef USER_CLOCK_SERVER_H_
#define USER_CLOCK_SERVER_H_

#define CLOCK_SERVER_NAME "CLOCK_SERVER"

int time(int tid);

int delay(int tid, int ticks);

int delayUntil(int tid, int ticks);

void clockServer();

namespace clock {
int time();
int delay(int ticks);
int delayUntil(int ticks);
}  // namespace clock

#endif  // USER_NAME_SERVER_H_