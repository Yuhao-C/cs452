#include "name_server.h"

#include "lib/bwio.h"
#include "lib/hashtable.h"
#include "lib/string.h"
#include "user/message.h"

#define MSG_LEN 32
#define MSG_REGISTER 0
#define MSG_WHO 1

void nameServer() {
  HashTable<String, int, 64, 10> nameTable;
  int senderTid;
  int namesIdx = 0;
  char names[64][MSG_LEN];
  char msg[MSG_LEN];
  int ret;
  while (true) {
    int receivedLen = receive(&senderTid, msg, MSG_LEN);
    assert(receivedLen == MSG_LEN);
    assert(msg[0] == MSG_REGISTER || msg[0] == MSG_WHO);
    if (msg[0] == MSG_REGISTER) {
      // register as
      strCopy(msg + 1, names[namesIdx], MSG_LEN);
      nameTable.put(names[namesIdx], senderTid);
      ++namesIdx;
      ret = 0;
      reply(senderTid, ret);
    } else {
      // who is
      const int *tid = nameTable.get(msg + 1);
      ret = tid ? *tid : -1;
      reply(senderTid, ret);
    }
  }
}

/**
 * @brief register current task as name
 *
 * @param name at most 30 characters
 * @return int
 */
int registerAs(const char *name) {
  char msg[MSG_LEN];
  int reply = -1;
  msg[0] = MSG_REGISTER;
  strCopy(name, msg + 1, MSG_LEN - 1);
  int status = send(NAME_SERVER_TID, msg, MSG_LEN, &reply, sizeof(int));
  return status >= 0 ? 0 : -1;
}

/**
 * @brief get tid from name
 *
 * @param name at most 30 characters
 * @return int
 */
int whoIs(const char *name) {
  char msg[36];
  int reply = -1;
  msg[0] = MSG_WHO;
  strCopy(name, msg + 1, MSG_LEN - 1);
  int status = send(NAME_SERVER_TID, msg, MSG_LEN, &reply, sizeof(int));
  return status >= 0 ? reply : -1;
}
