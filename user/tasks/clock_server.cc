#include "lib/assert.h"
#include "lib/bwio.h"
#include "name_server.h"
#include "user/message.h"

#define CLOCK_SERVER_NAME "CLOCK_SERVER"

enum Action { Time = 0, Delay, DelayUntil };

int time(int tid) {
  int currTime = -1;
  int msg[2] = {Action::Time, 0};
  send(tid, msg, currTime);
  return currTime;
}

int delay(int tid, int ticks) {
  if (ticks < 0) {
    return -2;
  }
  int currTime = -1;
  int msg[2] = {Action::Delay, ticks};
  send(tid, msg, currTime);
  return currTime;
}

int delayUntil(int tid, int ticks) {
  if (ticks < 0) {
    return -2;
  }
  int currTime = -1;
  int msg[2] = {Action::DelayUntil, ticks};
  send(tid, msg, currTime);
  return currTime;
}

void clockServer() {
  int senderTid;
  int request[2];

  registerAs(CLOCK_SERVER_NAME);

  while (true) {
    int receivedLen = receive(senderTid, request);
    assert(receivedLen == sizeof(request));
    int code = request[0];
    int payload = request[1];
    switch (code) {
      case Action::Time:
        break;
      case Action::Delay:
        break;
      case Action::DelayUntil:
        break;
      default:
        bwputstr(COM2, "[Clock Server]: fatal error unknown action\n\r");
        assert(false);
    }
  }
}