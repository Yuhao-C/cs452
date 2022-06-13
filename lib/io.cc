#include "lib/io.h"

#include "../user/include/uart_server.h"
#include "kern/arch/ts7200.h"
#include "lib/bwio.h"
#include "user/message.h"

int uart1Tid, uart2Tid;

void ioBootstrap(int _uart1Tid, int _uart2Tid) {
  uart1Tid = _uart1Tid;
  uart2Tid = _uart2Tid;
}

/*
 * The UARTs are initialized by RedBoot to the following state
 * 	115,200 bps
 * 	8 bits
 * 	no parity
 * 	fifos enabled
 */
int setfifo(unsigned int channel, int state) {
  volatile int *line = (int *)(channel + UART_LCRH_OFFSET);
  int buf = *line;
  buf = state ? buf | FEN_MASK : buf & ~FEN_MASK;
  *line = buf;
  return 0;
}

int setspeed(unsigned int channel, int speed) {
  volatile int *mid, *low;
  int baudDiv;
  mid = (int *)(channel + UART_LCRM_OFFSET);
  low = (int *)(channel + UART_LCRL_OFFSET);
  baudDiv = UARTCLK / (16 * speed) - 1;
  if (0 < baudDiv && baudDiv <= 0xffff) {
    *mid = (baudDiv >> 8) & 0xff;
    *low = baudDiv & 0xff;
    return 0;
  }
  return -1;
}

int setstp2(unsigned int channel, int select) {
  unsigned int base = channel;
  int *high, val;
  high = (int *)(base + UART_LCRH_OFFSET);
  val = *high;
  val = select ? val | STP2_MASK : val & ~STP2_MASK;
  *high = val;
  return 0;
}

int putc(unsigned int channel, char ch) {
  uart::Msg msg;
  msg.action = uart::Action::Putc;
  msg.data = ch;
  int retVal = -1;
  switch (channel) {
    case COM1:
      retVal = send(uart1Tid, msg);
      break;
    case COM2:
      retVal = send(uart2Tid, msg);
      break;
  }
  return retVal > -1 ? 0 : -1;
}

char c2x(char ch) {
  if ((ch <= 9)) return '0' + ch;
  return 'a' + ch - 10;
}

int putx(unsigned int channel, char c) {
  char chh, chl;

  chh = c2x(c / 16);
  chl = c2x(c % 16);
  putc(channel, chh);
  return putc(channel, chl);
}

int putr(unsigned int channel, unsigned int reg) {
  int byte;
  char *ch = (char *)&reg;

  for (byte = 3; byte >= 0; byte--) putx(channel, ch[byte]);
  return putc(channel, ' ');
}

int putstr(unsigned int channel, const char *str) {
  int count = 0;
  while (*str) {
    if (putc(channel, *str) < 0) return -1;
    ++str;
    ++count;
  }
  return count;
}

void putw(unsigned int channel, int n, char fc, const char *bf) {
  char ch;
  const char *p = bf;

  while (*p++ && n > 0) n--;
  while (n-- > 0) putc(channel, fc);
  while ((ch = *bf++)) putc(channel, ch);
}

int getc(unsigned int channel) {
  uart::Msg msg{uart::Action::Getc};
  char ch;
  int retVal = -1;
  switch (channel) {
    case COM1:
      retVal = send(uart1Tid, msg, ch);
      break;
    case COM2:
      retVal = send(uart2Tid, msg, ch);
      break;
  }
  return retVal > -1 ? ch : -1;
}

int a2d(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  return -1;
}

char a2i(char ch, const char **src, int base, int *nump) {
  int num, digit;
  const char *p;

  p = *src;
  num = 0;
  while ((digit = a2d(ch)) >= 0) {
    if (digit > base) break;
    num = num * base + digit;
    ch = *p++;
  }
  *src = p;
  *nump = num;
  return ch;
}

void ui2a(unsigned int num, unsigned int base, char *bf) {
  int n = 0;
  int dgt;
  unsigned int d = 1;

  while ((num / d) >= base) d *= base;
  while (d != 0) {
    dgt = num / d;
    num %= d;
    d /= base;
    if (n || dgt > 0 || d == 0) {
      *bf++ = dgt + (dgt < 10 ? '0' : 'a' - 10);
      ++n;
    }
  }
  *bf = 0;
}

void i2a(int num, char *bf) {
  if (num < 0) {
    num = -num;
    *bf++ = '-';
  }
  ui2a(num, 10, bf);
}

void format(unsigned int channel, const char *fmt, va_list va) {
  char bf[12];
  char ch, lz;
  int w;

  while ((ch = *(fmt++))) {
    if (ch != '%')
      putc(channel, ch);
    else {
      lz = 0;
      w = 0;
      ch = *(fmt++);
      switch (ch) {
        case '0':
          lz = '0';
          ch = *(fmt++);
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          if (!lz) lz = ' ';
          ch = a2i(ch, &fmt, 10, &w);
          break;
      }
      switch (ch) {
        case 0:
          return;
        case 'c':
          putc(channel, va_arg(va, char));
          break;
        case 's':
          putw(channel, w, 0, va_arg(va, char *));
          break;
        case 'u':
          ui2a(va_arg(va, unsigned int), 10, bf);
          putw(channel, w, lz, bf);
          break;
        case 'd':
          i2a(va_arg(va, int), bf);
          putw(channel, w, lz, bf);
          break;
        case 'x':
          ui2a(va_arg(va, unsigned int), 16, bf);
          putw(channel, w, lz, bf);
          break;
        case '%':
          putc(channel, ch);
          break;
      }
    }
  }
}

void printf(unsigned int channel, const char *fmt, ...) {
  va_list va;

  va_start(va, fmt);
  format(channel, fmt, va);
  va_end(va);
}

void println(unsigned int channel, const char *fmt, ...) {
  va_list va;

  va_start(va, fmt);
  format(channel, fmt, va);
  va_end(va);
  putstr(channel, "\n\r");
}
