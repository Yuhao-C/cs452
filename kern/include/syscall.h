#ifndef KERN_SYSCALL_H_
#define KERN_SYSCALL_H_

#define SYS_create 0
#define SYS_yield 1
#define SYS_exit 2
#define SYS_destroy 3

int taskCreate();  // TODO

int taskYield();

int taskExit();

int taskDestroy();

#endif  // KERN_SYSCALL_H_
