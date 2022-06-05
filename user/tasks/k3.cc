#include "k3.h"

#include "clock_server.h"
#include "kern/arch/ts7200.h"
#include "kern/sys.h"
#include "lib/bwio.h"
#include "name_server.h"
#include "user/message.h"
#include "user/task.h"

void k3Client() {
  int rply[2];
  int tid = myTid();
  send(0, 0, rply);
  int t = rply[0];
  int n = rply[1];
  int clockServerTid = whoIs(CLOCK_SERVER_NAME);
  for (int i = 0; i < n; ++i) {
    delay(clockServerTid, t);
    bwprintf(COM2, "tid: %d, delay interval: %d, delays completed: %d\n\r", tid,
             t, i + 1);
  }
}

void displayIdleStats() {
  int clockServerTid = whoIs(CLOCK_SERVER_NAME);
  unsigned int idleTime = getIdleTime();
  unsigned int sysTime = time(clockServerTid);
  bwprintf(COM2, "idle time: %u, sys time: %u, idle fraction: %u.%u%%\n\r",
           idleTime, sysTime, idleTime * 100 / sysTime,
           (idleTime * 1000 / sysTime) % 10);
}

void idleTask() {
  int tick = 0;
  while (true) {
    if (tick && tick % 50 == 0) {
      // executes every 50 ticks
      displayIdleStats();
    }
    *(volatile unsigned int *)HALT;  // halt until next interrupt
    ++tick;
  }
}