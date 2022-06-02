#ifndef KERN_MESSAGE_H_
#define KERN_MESSAGE_H_

#include "kern/syscall.h"

void msgSend();

void msgReceive();

void msgReply();

#endif  // KERN_MESSAGE_H_