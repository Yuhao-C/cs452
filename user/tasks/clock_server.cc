#include "clock_server.h"

#include "kern/syscall_code.h"
#include "lib/assert.h"
#include "lib/bwio.h"
#include "lib/heap.h"
#include "lib/timer.h"
#include "name_server.h"
#include "user/event.h"
#include "user/message.h"
#include "user/task.h"

enum Action { Time = 0, Delay, DelayUntil, Update };

struct DelayNode {
  int tid;
  int until;

  DelayNode() : tid{-1}, until{-1} {}
  DelayNode(int tid, int until) : tid{tid}, until{until} {}

  friend bool operator<(const DelayNode &l, const DelayNode &r) {
    return l.until < r.until;
  }
};

int time(int tid) {
  int currTime = -1;
  int msg[2] = {Action::Time, 0};
  if (send(tid, msg, currTime) >= 0) {
    return currTime;
  }
  return -1;
}

int delay(int tid, int ticks) {
  if (ticks < 0) {
    return -2;
  }
  int currTime = -1;
  int msg[2] = {Action::Delay, ticks};
  if (send(tid, msg, currTime) >= 0) {
    return currTime;
  }
  return -1;
}

int delayUntil(int tid, int ticks) {
  int currTime = -1;
  int msg[2] = {Action::DelayUntil, ticks};
  if (send(tid, msg, currTime) >= 0) {
    return currTime;
  }
  return -1;
}

void clockNotifier() {
  int serverTid = whoIs(CLOCK_SERVER_NAME);
  int msg[2] = {Action::Update, 0};
  int eventRet, rply;
  while (true) {
    eventRet = awaitEvent(IRQ_TC3UI);
    assert(eventRet == 0);
    ++msg[1];
    send(serverTid, msg, rply);
  }
}

void clockServer() {
  registerAs(CLOCK_SERVER_NAME);

  int tick = 0;
  int senderTid;
  int request[2];
  MinHeap<DelayNode, 64> delayHeap;

  create(0, clockNotifier);

  timer::stop(TIMER3_BASE);
  timer::load(TIMER3_BASE, 10);
  timer::start(TIMER3_BASE);

  while (true) {
    int receivedLen = receive(senderTid, request);
    assert(receivedLen == sizeof(request));

    int code = request[0];
    int payload = request[1];

    switch (code) {
      case Action::Update: {
        tick = payload;
        reply(senderTid, 0);
        const DelayNode *node = delayHeap.peekMin();
        while (node && node->until <= tick) {
          reply(node->tid, tick);
          delayHeap.deleteMin();
          node = delayHeap.peekMin();
        }
        break;
      }
      case Action::Time:
        reply(senderTid, tick);
        break;
      case Action::Delay:
        assert(payload >= 0);
        delayHeap.insert({senderTid, tick + payload});
        break;
      case Action::DelayUntil:
        if (payload >= 0) {
          delayHeap.insert({senderTid, payload});
        } else {
          reply(senderTid, -2);
        }
        break;
      default:
        bwputstr(COM2, "[Clock Server]: fatal error unknown action\n\r");
        assert(false);
    }
  }
}