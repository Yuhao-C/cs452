#ifndef USER_UART_SERVER_H_
#define USER_UART_SERVER_H_

#define UART1_SERVER_NAME "UART1_SERVER"
#define UART2_SERVER_NAME "UART2_SERVER"

namespace uart {

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

void server();
void sendNotifier();
void recvNotifier();
void bootstrap();
}  // namespace uart

#endif  // USER_UART_SERVER_H_