#include "k3.h"

#include "clock_server.h"
#include "kern/arch/ts7200.h"
#include "kern/sys.h"
#include "lib/io.h"
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
    println(COM2, "tid: %d, delay interval: %d, delays completed: %d", tid, t,
            i + 1);
  }
}

void idleTask() {
  while (true) {
    *(volatile unsigned int *)HALT;  // halt until next interrupt
  }
}