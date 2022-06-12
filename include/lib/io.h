#ifndef KERN_LIB_IO_H_
#define KERN_LIB_IO_H_

#include "kern/arch/ts7200.h"

void ioBootstrap(int _uart1Tid, int _uart2Tid);

int setfifo(unsigned int channel, int state);

int setspeed(unsigned int channel, int speed);

int setstp2(unsigned int channel, int select);

char a2i(char ch, const char **src, int base, int *nump);

int putc(unsigned int channel, char ch);

int getc(unsigned int channel);

int putx(unsigned int channel, char c);

int putstr(unsigned int channel, const char *str);

int putr(unsigned int channel, unsigned int reg);

void putw(unsigned int channel, int n, char fc, const char *bf);

void printf(unsigned int channel, const char *fmt, ...);

void println(unsigned int channel, const char *fmt, ...);

#endif  // KERN_LIB_IO_H_