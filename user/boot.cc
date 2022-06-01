#include "boot.h"

#include "k1.h"
#include "lib/bwio.h"
#include "lib/timer.h"
#include "name_server.h"
#include "perf_test.h"
#include "rps.h"
#include "user/message.h"
#include "user/task.h"

void startRps() {
  create(2, rps::server);
  create(3, rps::player1);
  create(3, rps::player1);
  create(3, rps::player2);
  create(3, rps::player3);
  create(3, rps::player1);
}

void startPerfTest() {
#if SENDER_FIRST
  perf_test::senderFirst();
#else
  perf_test::receiverFirst();
#endif
}

void idleTask() {
  while (true) {
    (void)0;
  }
}

void boot() {
  timer::stop();
  timer::load();
  timer::start();
  create(0, nameServer);

  startRps();
}
