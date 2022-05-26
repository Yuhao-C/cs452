#ifndef USER_MESSAGE_H_
#define USER_MESSAGE_H_

extern "C" {
int send(int tid, const char *msg, int msgLen, char *reply, int replyLen);

int receive(int *tid, char *msg, int msgLen);

int reply(int tid, const char *reply, int replyLen);
}

#endif  // USER_MESSAGE_H_
