#ifndef LIB_ASYNC_MSG_H_
#define LIB_ASYNC_MSG_H_

#include "user/message.h"
#include "user/task.h"

template <typename M>
void sendWorker() {
  int parentTid, recvTid;
  M msg;
  receive(parentTid, recvTid);
  reply(parentTid);
  receive(parentTid, msg);
  reply(parentTid);
  send(recvTid, msg);
}

template <typename M>
void asyncSend(int tid, const M &msg) {
  int worker = create(4, sendWorker<M>);
  send(worker, tid);
  send(worker, msg);
}

#endif  // LIB_ASYNC_MSG_H_
