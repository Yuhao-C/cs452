#ifndef KERN_TASK_H_
#define KERN_TASK_H_

#include "common.h"

int create(int priority, void (*function)());

int myTid();

int myParentTid();

void yield();

void exit();

void destroy();

#endif  // KERN_TASK_H_
