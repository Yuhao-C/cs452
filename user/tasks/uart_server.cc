#include "uart_server.h"

#include "kern/arch/ts7200.h"
#include "kern/syscall_code.h"
#include "lib/bwio.h"
#include "lib/queue.h"
#include "name_server.h"
#include "user/event.h"
#include "user/message.h"
#include "user/task.h"

namespace uart {

const int BUFFER_SIZE = 16;

enum Action {
  Getc,
  Putc,
  Recv,
  Send,
};

struct Msg {
  Action action;
  char data;
};

struct ServerArgs {
  unsigned int channel;
  bool fifo;
  int speed;
  bool stp2;
  int eventType;
  bool cts;
  const char *name;
};

struct RecvNotifierArgs {
  unsigned int channel;
  int eventType;
};

struct SendNotifierArgs {
  unsigned int channel;
  int eventType;
  bool cts;
};

int tryRecv(unsigned int channel, char *dst) {
  volatile unsigned int *flags, *data;
  int count = 0;

  flags = (unsigned int *)(channel + UART_FLAG_OFFSET);
  data = (unsigned int *)(channel + UART_DATA_OFFSET);

  while (!(*flags & RXFE_MASK) && count < BUFFER_SIZE) {
    dst[count] = *data;
    ++count;
  }
  return count;
}

void init(const ServerArgs &args) {
  bwsetfifo(args.channel, args.fifo);
  bwsetspeed(args.channel, args.speed);
  bwsetstp2(args.channel, args.stp2);
  registerAs(args.name);
}

void recvNotifier() {
  int serverTid;
  RecvNotifierArgs args;
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
  SendNotifierArgs args;
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
      if (status & TIEN_MASK) {
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

  RecvNotifierArgs recvArgs{args.channel, args.eventType};
  int recvNotifierTid = create(0, recvNotifier);
  send(recvNotifierTid, recvArgs);

  SendNotifierArgs sendArgs{args.channel, args.eventType, args.cts};
  int sendNotifierTid = create(0, sendNotifier);
  send(sendNotifierTid, sendArgs);

  Queue<char, 8192> recvBuffer;
  Queue<char, 8192> sendBuffer;
  Queue<int, 64> getcRequestors;

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

        while (sendBuffer.size() > 0 && !(*flags & TXFF_MASK)) {
          *data = sendBuffer.dequeue();
        }
        if (*flags & TXFF_MASK) {
          reply(sendNotifierTid);
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
        while (sendBuffer.size() > 0 && !(*flags & TXFF_MASK)) {
          *data = sendBuffer.dequeue();
        }
        if (*flags & TXFF_MASK) {
          reply(senderTid);
        }
        break;
    }
  }
}

void bootstrap() {
  int uart1Tid = create(0, server);
  int uart2Tid = create(0, server);

  ServerArgs args1{COM1, false, 2400, true, IRQ_UART1, true, UART1_SERVER_NAME};
  ServerArgs args2{COM2,      true,  115200,           false,
                   IRQ_UART2, false, UART2_SERVER_NAME};

  send(uart1Tid, args1);
  send(uart2Tid, args2);
}

}  // namespace uart

int getc(int tid) {
  uart::Msg msg{uart::Action::Getc};
  char ch;
  int retVal = send(tid, msg, ch);
  return retVal > -1 ? ch : -1;
}

int putc(int tid, char ch) {
  uart::Msg msg;
  msg.action = uart::Action::Putc;
  msg.data = ch;
  return send(tid, msg);
}