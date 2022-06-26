#include "clock_server.h"
#include "display_server.h"
#include "kern/sys.h"
#include "name_server.h"
#include "user/message.h"

void stats() {
  int displayServerTid = whoIs(DISPLAY_SERVER_NAME);
  int clockServerTid = whoIs(CLOCK_SERVER_NAME);
  int t = 0;

  while (true) {
    unsigned int idleTime = getIdleTime();
    unsigned int sysTime = time(clockServerTid);
    view::Msg msg{view::Action::Time, {sysTime, idleTime}};
    send(displayServerTid, msg);
    clock::delayUntil(t += 10);
  }
}