#ifndef KERN_SYS_H_
#define KERN_SYS_H_

#include "kern/common.h"

void sysBootstrap(addr_t lr);

void kExit();

#endif  // KERN_SYS_H_