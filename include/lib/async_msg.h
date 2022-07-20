#ifndef LIB_ASYNC_MSG_H_
#define LIB_ASYNC_MSG_H_

#include "clock_server.h"
#include "user/message.h"
#include "user/task.h"

template <typename M>
void sendWorker() {
  int parentTid, recvTid, ticks;
  M msg;
  receive(parentTid, recvTid);
  reply(parentTid);
  receive(parentTid, msg);
  reply(parentTid);
  receive(parentTid, ticks);
  reply(parentTid);
  clock::delay(ticks);
  send(recvTid, msg);
}

template <typename M>
void delaySend(int tid, const M &msg, int ticks) {
  int worker = create(4, sendWorker<M>);
  send(worker, tid);
  send(worker, msg);
  send(worker, ticks);
}

template <typename M>
void asyncSend(int tid, const M &msg) {
  delaySend(tid, msg, 0);
}

#endif  // LIB_ASYNC_MSG_H_
