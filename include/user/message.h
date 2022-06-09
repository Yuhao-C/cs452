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

template <typename M>
int send(int tid, const M &msg) {
  return send(tid, &msg, sizeof(M), nullptr, 0);
}

template <typename M>
int receive(int &tid, M &msg) {
  return receive(&tid, &msg, sizeof(M));
}

template <typename R>
int reply(int tid, const R &rply) {
  return reply(tid, &rply, sizeof(R));
}

inline int reply(int tid) { return reply(tid, nullptr, 0); }

#endif  // USER_MESSAGE_H_
