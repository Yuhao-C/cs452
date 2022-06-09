#include "boot.h"

#include "clock_server.h"
#include "k1.h"
#include "k3.h"
#include "lib/bwio.h"
#include "lib/timer.h"
#include "name_server.h"
#include "perf_test.h"
#include "rps.h"
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

  create(7, idleTask);

  uart::bootstrap();

  int uart1Tid = whoIs(UART1_SERVER_NAME);
  int uart2Tid = whoIs(UART2_SERVER_NAME);
  int clockServerTid = whoIs(CLOCK_SERVER_NAME);

  putc(uart1Tid, 10);
  putc(uart1Tid, 24);

  delay(clockServerTid, 15);

  for (int i = 1; i < 8; ++i) {
    putc(uart1Tid, 34);
    putc(uart1Tid, i);
    delay(clockServerTid, 15);
  }
  putc(uart1Tid, 32);

  delay(clockServerTid, 500);

  putc(uart1Tid, 0);
  putc(uart1Tid, 24);

  // int rply[4][2] = {{10, 20}, {23, 9}, {33, 6}, {71, 3}};
  // for (int i = 3; i <= 6; ++i) {
  //   create(i, k3Client);
  // }

  // for (int i = 0; i < 4; ++i) {
  //   int tid, msg;
  //   receive(tid, msg);
  //   reply(tid, rply[i]);
  // }
}
