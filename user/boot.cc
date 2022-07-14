#include "boot.h"

#include "clock_server.h"
#include "console.h"
#include "display_server.h"
#include "k1.h"
#include "k3.h"
#include "lib/timer.h"
#include "marklin/reservation.h"
#include "marklin/routing.h"
#include "marklin/world.h"
#include "marklin_server.h"
#include "name_server.h"
#include "perf_test.h"
#include "rps.h"
#include "stats.h"
#include "uart_server.h"
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

void boot() {
  create(0, nameServer);
  create(0, clockServer);
  uart::bootstrap();

  create(1, marklin::cmdServer);

  create(2, marklin::ReservationServer::runServer);

  create(1, marklin::runWorld);

  create(2, marklin::runRouting);

  create(2, view::displayServer);
  create(2, consoleReader);

  create(3, stats);

  create(7, idleTask);
}
