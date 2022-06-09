#ifndef USER_UART_SERVER_H_
#define USER_UART_SERVER_H_

#define UART1_SERVER_NAME "UART1_SERVER"
#define UART2_SERVER_NAME "UART2_SERVER"

namespace uart {
void server();
void sendNotifier();
void recvNotifier();
void bootstrap();
}  // namespace uart

int getc(int tid);
int putc(int tid, char ch);

#endif  // USER_UART_SERVER_H_