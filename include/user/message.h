#ifndef USER_MESSAGE_H_
#define USER_MESSAGE_H_

extern "C" {
int send(int tid, const void *msg, int msgLen, void *reply, int replyLen);

int receive(int *tid, void *msg, int msgLen);

int reply(int tid, const void *reply, int replyLen);
}

template <typename M, typename R>
int send(int tid, const M &msg, R &reply) {
  return send(tid, &msg, sizeof(M), &reply, sizeof(R));
}

#endif  // USER_MESSAGE_H_
