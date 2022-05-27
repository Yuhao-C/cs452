#ifndef USER_MESSAGE_H_
#define USER_MESSAGE_H_

extern "C" {
int send(int tid, const void *msg, int msgLen, void *reply, int replyLen);

int receive(int *tid, void *msg, int msgLen);

int reply(int tid, const void *reply, int replyLen);
}

#endif  // USER_MESSAGE_H_
