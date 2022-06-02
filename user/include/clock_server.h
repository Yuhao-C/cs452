#ifndef USER_CLOCK_SERVER_H_
#define USER_CLOCK_SERVER_H_

#define CLOCK_SERVER_NAME "CLOCK_SERVER"

int time(int tid);

int delay(int tid, int ticks);

int delayUntil(int tid, int ticks);

void clockServer();

#endif  // USER_NAME_SERVER_H_