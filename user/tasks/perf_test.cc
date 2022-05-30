#include "perf_test.h"

#include "lib/assert.h"
#include "lib/bwio.h"
#include "lib/timer.h"
#include "user/message.h"
#include "user/task.h"

#define TEN(e) \
  e;           \
  e;           \
  e;           \
  e;           \
  e;           \
  e;           \
  e;           \
  e;           \
  e;           \
  e;
#define HUNDRED(e) TEN(TEN(e))
#define THOUSAND(e) HUNDRED(TEN(e))

namespace perf_test {

unsigned int timerOverhead;

void timerTest() {
  unsigned int t0, t1;
  t0 = timer::getTick();
  THOUSAND(t1 = timer::getTick());
  timerOverhead = t0 - t1;
  bwprintf(COM2, "timer overhead: %dt\r\n", timerOverhead);
}

void sender() {
  int messageSize[] = {4, 64, 256};
  char msg[256];
  char reply[256];
  for (int i = 0; i < 256; ++i) {
    msg[i] = 'A';
  }
  int receiverTid = 3;

#if ENABLE_OPT
  char opt[] = "opt";
#else
  char opt[] = "noopt";
#endif

#if ENABLE_CACHE
  char cch[] = "cache";
#else
  char cch[] = "nocache";
#endif

#if SENDER_FIRST
  char mode = 'S';
#else
  char mode = 'R';
#endif

  for (int i = 0; i < 3; ++i) {
    int size = messageSize[i];
    // bwprintf(COM2, "sender start sending 1000 %d-byte messages\n\r", size);
    unsigned int t0 = timer::getTick();
    THOUSAND(send(receiverTid, msg, size, reply, size));
    unsigned int t1 = timer::getTick();
    (void)reply;
    bwprintf(COM2, "%s %s %c %d %d\n\r", opt, cch, mode, size,
             (t0 - t1 - timerOverhead) / 508);
  }
}

void receiver() {
  int messageSize[] = {4, 64, 256};
  char msg[256];
  char replyMsg[256];
  for (int i = 0; i < 256; ++i) {
    replyMsg[i] = 'B';
  }
  int senderTid = -1;
  for (int i = 0; i < 3; ++i) {
    int size = messageSize[i];
    // bwprintf(COM2, "receiver start waiting for %d bytes\n\r", size);
    THOUSAND(receive(&senderTid, msg, size); reply(senderTid, replyMsg, size););
  }
}

void senderFirst() {
  timerTest();
  create(2, sender);
  create(3, receiver);
}

void receiverFirst() {
  timerTest();
  create(3, sender);
  create(2, receiver);
}

}  // namespace perf_test