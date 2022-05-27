#ifndef USER_NAME_SERVER_H_
#define USER_NAME_SERVER_H_

#define NAME_SERVER_TID 1

void nameServer();

int registerAs(const char *name);

int whoIs(const char *name);

#endif  // USER_NAME_SERVER_H_