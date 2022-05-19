#ifndef USER_TASK_H_
#define USER_TASK_H_

extern "C" {
int create(int priority, void (*function)());

int myTid();

int myParentTid();

void yield();

void exit();
}
#endif  // USER_TASK_H_
