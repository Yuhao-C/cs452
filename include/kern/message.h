#ifndef KERN_MESSAGE_H_
#define KERN_MESSAGE_H_

#include "kern/syscall.h"

void msgSend(Trapframe *tf);

void msgReceive(Trapframe *tf);

void msgReply(Trapframe *tf);

#endif  // KERN_MESSAGE_H_