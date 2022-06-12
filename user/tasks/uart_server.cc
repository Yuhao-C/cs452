#include "uart_server.h"

#include "kern/arch/ts7200.h"
#include "kern/syscall_code.h"
#include "lib/io.h"
#include "lib/queue.h"
#include "name_server.h"
#include "user/event.h"
#include "user/message.h"
#include "user/task.h"

namespace uart {

const int BUFFER_SIZE = 16;

struct ServerArgs {
  unsigned int channel;
  bool fifo;
  int speed;
  bool stp2;
  int eventType;
  bool cts;
  const char *name;
};

void init(const ServerArgs &args) {
  setfifo(args.channel, args.fifo);
  setspeed(args.channel, args.speed);
  setstp2(args.channel, args.stp2);
  registerAs(args.name);
}

void recvNotifier() {
  int serverTid;
  ServerArgs args;
  receive(serverTid, args);
  reply(serverTid);

  Msg msg;
  msg.action = Recv;

  volatile unsigned int *flags =
      (unsigned int *)(args.channel + UART_FLAG_OFFSET);
  volatile unsigned int *ctrl =
      (unsigned int *)(args.channel + UART_CTRL_OFFSET);

  while (true) {
    if (!(*flags & RXFE_MASK)) {
      send(serverTid, msg);
      continue;
    }

    // enable and wait for RX and RT interrupt
    *ctrl |= RIEN_MASK | RTIEN_MASK;
    while (true) {
      int status = awaitEvent(args.eventType);
      if (status & RIS_MASK || status & RTIS_MASK) {
        break;
      }
    }
  }
}

void sendNotifier() {
  int serverTid;
  ServerArgs args;
  receive(serverTid, args);
  reply(serverTid);

  Msg msg;
  msg.action = Send;

  volatile unsigned int *flags =
      (unsigned int *)(args.channel + UART_FLAG_OFFSET);
  volatile unsigned int *ctrl =
      (unsigned int *)(args.channel + UART_CTRL_OFFSET);

  while (true) {
    if (!(*flags & TXFF_MASK)) {
      send(serverTid, msg);
      continue;
    }

    // enable and wait for TX interrupt
    *ctrl |= TIEN_MASK;
    while (true) {
      int status = awaitEvent(args.eventType);
      if (status & TIS_MASK) {
        break;
      }
    }
  }
}

void sendNotifierCTS() {
  int serverTid;
  ServerArgs args;
  receive(serverTid, args);
  reply(serverTid);

  Msg msg;
  msg.action = Send;

  volatile unsigned int *flags =
      (unsigned int *)(args.channel + UART_FLAG_OFFSET);
  volatile unsigned int *ctrl =
      (unsigned int *)(args.channel + UART_CTRL_OFFSET);

  bool cts = (*flags & CTS_MASK);
  bool ctsReasserted = cts;

  while (true) {
    if (!(*flags & TXFF_MASK) && ctsReasserted) {
      ctsReasserted = false;
      send(serverTid, msg);
    }

    // enable and wait for TX interrupt
    *ctrl |= TIEN_MASK;
    while (true) {
      int status = awaitEvent(args.eventType);
      if (status & MIS_MASK) {
        bool newCts = *flags & CTS_MASK;
        if (!cts && newCts) {
          ctsReasserted = true;
        }
        cts = newCts;
      }
      if (ctsReasserted) {
        break;
      }
    }
  }
}

void server() {
  int senderTid;
  ServerArgs args;
  receive(senderTid, args);
  reply(senderTid);
  init(args);

  int recvNotifierTid = create(0, recvNotifier);
  send(recvNotifierTid, args);

  int sendNotifierTid = create(0, args.cts ? sendNotifierCTS : sendNotifier);
  send(sendNotifierTid, args);

  Queue<char, 8192> recvBuffer;
  Queue<char, 8192> sendBuffer;
  Queue<int, 64> getcRequestors;

  bool ctsCanSend = false;

  volatile unsigned int *flags =
      (unsigned int *)(args.channel + UART_FLAG_OFFSET);
  volatile unsigned int *data =
      (unsigned int *)(args.channel + UART_DATA_OFFSET);
  while (true) {
    Msg msg;
    int result;
    receive(senderTid, msg);
    switch (msg.action) {
      case Getc:
        getcRequestors.enqueue(senderTid);
        if (recvBuffer.size() > 0) {
          reply(getcRequestors.dequeue(), recvBuffer.dequeue());
        }
        break;
      case Putc:
        result = sendBuffer.enqueue(msg.data);
        assert(result == true);
        reply(senderTid);

        if (args.cts) {
          if (ctsCanSend && !(*flags & TXFF_MASK)) {
            reply(sendNotifierTid);
            *data = sendBuffer.dequeue();
            ctsCanSend = false;
          }
        } else {
          while (sendBuffer.size() > 0 && !(*flags & TXFF_MASK)) {
            *data = sendBuffer.dequeue();
          }
          if (*flags & TXFF_MASK) {
            reply(sendNotifierTid);
          }
        }
        break;
      case Recv:
        while (!(*flags & RXFE_MASK)) {
          char c = *data;
          result = recvBuffer.enqueue(c);
          assert(result == true);
        }
        reply(senderTid);

        while (getcRequestors.size() > 0 && recvBuffer.size() > 0) {
          reply(getcRequestors.dequeue(), recvBuffer.dequeue());
        }
        break;
      case Send:
        if (args.cts) {
          if (sendBuffer.size() > 0 && !(*flags & TXFF_MASK)) {
            reply(senderTid);
            *data = sendBuffer.dequeue();
          } else {
            ctsCanSend = true;
          }
        } else {
          while (sendBuffer.size() > 0 && !(*flags & TXFF_MASK)) {
            *data = sendBuffer.dequeue();
          }
          if (*flags & TXFF_MASK) {
            reply(senderTid);
          }
        }
        break;
    }
  }
}

void bootstrap() {
  int uart1Tid = create(0, server);
  int uart2Tid = create(0, server);

  ioBootstrap(uart1Tid, uart2Tid);

  ServerArgs args1{COM1, false, 2400, true, IRQ_UART1, true, UART1_SERVER_NAME};
  ServerArgs args2{COM2,      true,  115200,           false,
                   IRQ_UART2, false, UART2_SERVER_NAME};

  send(uart1Tid, args1);
  send(uart2Tid, args2);
}

}  // namespace uart
