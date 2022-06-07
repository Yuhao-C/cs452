#include "uart_server.h"

#include "kern/arch/ts7200.h"
#include "lib/bwio.h"
namespace uart {

void init() {
  // enable all UART interrupts
  *(volatile unsigned int *)(UART1_BASE + UART_CTRL_OFFSET) = 0b01111001;
  *(volatile unsigned int *)(UART2_BASE + UART_CTRL_OFFSET) = 0b01111001;

  bwsetfifo(COM1, OFF);
  bwsetfifo(COM2, ON);

  bwsetspeed(COM1, 2400);
  bwsetspeed(COM2, 115200);

  bwsetstp2(COM1, ON);
  bwsetstp2(COM2, OFF);
}

void server() { init(); }

}  // namespace uart